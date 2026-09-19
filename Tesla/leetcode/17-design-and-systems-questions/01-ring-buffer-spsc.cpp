// COMPONENT   A lock-free single-producer / single-consumer ring buffer.
// WHERE       The handoff between a sensor/driver thread and a processing thread. Every
//             autonomy stack has several of these.
// WHY IT WINS No mutex, so no unbounded blocking and no priority inversion; fixed
//             capacity, so no allocation and a bounded worst case; contiguous storage, so
//             it is cache friendly.
//
// DESIGN DECISIONS, each of which an interviewer will probe:
//   - POWER-OF-TWO capacity so the wrap is a mask, not a modulo (a division is ~20-40
//     cycles; the AND is 1).
//   - One slot is always left empty so head == tail unambiguously means EMPTY. The
//     alternative (a separate `full` flag) costs another shared variable.
//   - head_ and tail_ are on SEPARATE CACHE LINES. Without that, the producer's write to
//     head_ invalidates the consumer's line holding tail_ on every push -- false sharing,
//     measured at ~5x in the course section 11 benchmark.
//   - Memory ordering: each index is written by exactly ONE thread, so that thread reads
//     its own index relaxed; reading the OTHER thread's index is an ACQUIRE, and
//     publishing your own is a RELEASE. That release/acquire pair is what makes the plain
//     (non-atomic) element access race-free.
//   - try_push / try_pop NEVER BLOCK: they return false when full/empty. A real-time
//     producer must not be able to stall on a slow consumer.
//
// COMPLEXITY  push and pop are O(1), wait-free, with no allocation after construction.
//
// FOLLOW-UPS  Multi-producer? -> a CAS loop on the head, which is lock-free but no longer
//             wait-free; or a per-producer ring plus a combiner. Say that SPSC is
//             fundamentally cheaper and prefer it in the design.
//             What if the consumer falls behind? -> an explicit OVERFLOW POLICY. Here
//             push fails and the caller counts a drop; a "drop oldest" variant is also
//             defensible, but silently losing data is not.
//             Blocking variants? -> add a std::counting_semaphore or atomic wait/notify,
//             but keep the non-blocking path for the RT thread.
//             Testing? -> a single-threaded model test against a std::deque, plus a
//             multi-threaded stress test under TSan (both below).

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <thread>
#include <type_traits>
#include <vector>

template <typename T, std::size_t Capacity>
class SpscRing {
    static_assert(Capacity >= 2, "need at least two slots (one is kept empty)");
    static_assert((Capacity & (Capacity - 1)) == 0, "capacity must be a power of two");
    static_assert(std::is_trivially_copyable_v<T> || std::is_move_assignable_v<T>);

    static constexpr std::size_t kMask = Capacity - 1;
    static constexpr std::size_t kLine = 64;          // hardware_destructive_interference_size

    alignas(kLine) std::atomic<std::size_t> head_{0};   // written by the PRODUCER only
    alignas(kLine) std::atomic<std::size_t> tail_{0};   // written by the CONSUMER only
    alignas(kLine) std::array<T, Capacity> slots_{};

public:
    static constexpr std::size_t capacity() noexcept { return Capacity - 1; }

    // Producer thread only. Never blocks; false means full.
    bool try_push(const T& value) noexcept {
        const std::size_t head = head_.load(std::memory_order_relaxed);   // our own index
        const std::size_t next = (head + 1) & kMask;
        if (next == tail_.load(std::memory_order_acquire)) return false;  // full
        slots_[head] = value;                                              // plain write
        head_.store(next, std::memory_order_release);   // publishes the write above
        return true;
    }

    // Consumer thread only. Never blocks; nullopt means empty.
    bool try_pop(T& out) noexcept {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);   // our own index
        if (tail == head_.load(std::memory_order_acquire)) return false;  // empty
        out = slots_[tail];                                                // plain read
        tail_.store((tail + 1) & kMask, std::memory_order_release);  // frees the slot
        return true;
    }

    std::optional<T> try_pop() noexcept {
        T value{};
        return try_pop(value) ? std::optional<T>{value} : std::nullopt;
    }

    // Approximate: safe to call from either thread, but the answer may be stale.
    std::size_t size_approx() const noexcept {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail) & kMask;
    }
    bool empty_approx() const noexcept { return size_approx() == 0; }
};

int main() {
    // ---- single-threaded behaviour, including the boundaries -------------------
    SpscRing<int, 4> ring;                    // 4 slots -> 3 usable
    static_assert(SpscRing<int, 4>::capacity() == 3);
    assert(ring.empty_approx());
    assert(!ring.try_pop().has_value());      // pop from empty

    assert(ring.try_push(1) && ring.try_push(2) && ring.try_push(3));
    assert(!ring.try_push(4));                // FULL: one slot is deliberately unused
    assert(ring.size_approx() == 3);

    assert(ring.try_pop() == 1);              // FIFO order
    assert(ring.try_push(4));                 // a slot freed up
    assert(ring.try_pop() == 2);
    assert(ring.try_pop() == 3);
    assert(ring.try_pop() == 4);
    assert(!ring.try_pop().has_value());
    assert(ring.empty_approx());

    // Wrap around many times: the mask must keep working.
    for (int round = 0; round < 1000; ++round) {
        assert(ring.try_push(round));
        assert(ring.try_pop() == round);
    }

    // ---- model-based test against a reference implementation -------------------
    {
        SpscRing<int, 8> r;
        std::vector<int> model;                 // the reference: a plain FIFO
        std::uint32_t rng = 12345;
        for (int step = 0; step < 20000; ++step) {
            rng = rng * 1664525u + 1013904223u;            // a deterministic LCG
            if ((rng >> 16) & 1) {
                const int v = static_cast<int>(rng & 0xFF);
                const bool pushed = r.try_push(v);
                assert(pushed == (model.size() < r.capacity()));
                if (pushed) model.push_back(v);
            } else {
                const auto popped = r.try_pop();
                assert(popped.has_value() == !model.empty());
                if (popped) { assert(*popped == model.front()); model.erase(model.begin()); }
            }
            assert(r.size_approx() == model.size());
        }
    }

    // ---- concurrent stress test (run this file under -fsanitize=thread) --------
    {
        constexpr int kItems = 200000;
        SpscRing<int, 1024> r;
        std::atomic<bool> done{false};
        long long checksum = 0;

        std::thread consumer{[&] {
            int received = 0;
            int value = 0;
            while (received < kItems) {
                if (r.try_pop(value)) {
                    assert(value == received);       // strict FIFO, nothing lost
                    checksum += value;
                    ++received;
                } else if (done.load(std::memory_order_acquire) && r.empty_approx()) {
                    break;
                }
            }
        }};

        for (int i = 0; i < kItems; ++i)
            while (!r.try_push(i)) std::this_thread::yield();   // back-pressure, no block
        done.store(true, std::memory_order_release);
        consumer.join();

        const long long expected = static_cast<long long>(kItems - 1) * kItems / 2;
        assert(checksum == expected);
    }
    return 0;
}
