// 11 — Threads, mutexes, atomics, the memory model.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -O2 -pthread -g examples.cpp -o ex && ./ex
//   Race detector:  g++ -std=c++20 -O1 -g -pthread -fsanitize=thread ... && ./ex

#include <atomic>
#include <barrier>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <latch>
#include <mutex>
#include <new>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ------------------------------------------------------------------- threads
static void threads() {
    std::atomic<int> total{0};

    {   // std::jthread joins in its destructor and supports cooperative cancellation.
        std::jthread worker{[&total](std::stop_token stop) {
            while (!stop.stop_requested()) {
                total.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(100us);
            }
        }};
        std::this_thread::sleep_for(5ms);
        worker.request_stop();
    }   // ~jthread: request_stop() + join(). A raw std::thread here would terminate.
    assert(total.load() > 0);

    // hardware_concurrency is a HINT and may return 0.
    const unsigned hw = std::thread::hardware_concurrency();
    std::cout << "  hardware_concurrency = " << hw << '\n';
}

// ------------------------------------------------------ mutex & lock ordering
struct Account {
    std::mutex m;
    int balance;
};

// WRONG: locking in argument order deadlocks when two transfers cross.
//   std::lock_guard a{from.m}; std::lock_guard b{to.m};
// RIGHT: one atomic multi-lock with a deadlock-avoidance algorithm.
static void transfer(Account& from, Account& to, int amount) {
    std::scoped_lock lock{from.m, to.m};       // C++17: std::lock() under the hood
    from.balance -= amount;
    to.balance   += amount;
}

static void mutexes() {
    Account a{{}, 100}, b{{}, 100};
    // Two threads transferring in OPPOSITE directions is exactly the deadlock case.
    std::jthread t1{[&] { for (int i = 0; i < 1000; ++i) transfer(a, b, 1); }};
    std::jthread t2{[&] { for (int i = 0; i < 1000; ++i) transfer(b, a, 1); }};
    t1.join();
    t2.join();
    assert(a.balance + b.balance == 200);      // conserved, and no deadlock

    // shared_mutex: many readers OR one writer.
    std::shared_mutex sm;
    int shared_value = 0;
    {
        std::unique_lock w{sm};                 // exclusive
        shared_value = 7;
    }
    {
        std::shared_lock r1{sm}, r2{sm};        // two readers at once: fine
        assert(shared_value == 7);
    }

    // std::call_once, and its simpler equivalent.
    std::once_flag flag;
    int initialized = 0;
    auto init = [&] { std::call_once(flag, [&] { ++initialized; }); };
    { std::jthread x{init}, y{init}, z{init}; }
    assert(initialized == 1);
}

// ------------------------------------------ condition variable (correctly)
template <typename T>
class BlockingQueue {
    mutable std::mutex m_;
    std::condition_variable cv_;
    std::queue<T> q_;
    bool closed_{false};
public:
    void push(T v) {
        { std::lock_guard lk{m_}; q_.push(std::move(v)); }
        cv_.notify_one();                       // notify outside the lock
    }
    // Returns nullopt only when the queue is closed AND drained.
    std::optional<T> pop() {
        std::unique_lock lk{m_};
        // PREDICATE form: immune to spurious wakeups and to a notify that arrived
        // before we started waiting.
        cv_.wait(lk, [this] { return !q_.empty() || closed_; });
        if (q_.empty()) return std::nullopt;
        T v = std::move(q_.front());
        q_.pop();
        return v;
    }
    void close() {
        { std::lock_guard lk{m_}; closed_ = true; }
        cv_.notify_all();                       // wake EVERY waiter, not one
    }
};

static void condition_variables() {
    BlockingQueue<int> q;
    std::atomic<int> consumed{0};
    constexpr int kItems = 500;

    std::vector<std::jthread> consumers;
    for (int i = 0; i < 4; ++i)
        consumers.emplace_back([&] {
            while (auto item = q.pop()) consumed.fetch_add(*item, std::memory_order_relaxed);
        });

    for (int i = 1; i <= kItems; ++i) q.push(i);
    q.close();
    consumers.clear();                          // joins all four

    assert(consumed.load() == kItems * (kItems + 1) / 2);
}

// ------------------------------------------------------------------ atomics
static void atomics() {
    static_assert(std::atomic<int>::is_always_lock_free);
    static_assert(std::atomic<void*>::is_always_lock_free);
    // A large struct is NOT lock free -- std::atomic silently uses an internal mutex.
    struct Big { double a[8]; };
    static_assert(!std::atomic<Big>::is_always_lock_free);

    std::atomic<int> counter{0};
    constexpr int kThreads = 4, kPerThread = 10'000;
    {
        std::vector<std::jthread> ts;
        for (int i = 0; i < kThreads; ++i)
            ts.emplace_back([&] {
                for (int n = 0; n < kPerThread; ++n)
                    // relaxed is correct HERE: we only need atomicity, and nobody
                    // reads the value until every thread has joined.
                    counter.fetch_add(1, std::memory_order_relaxed);
            });
    }
    assert(counter.load() == kThreads * kPerThread);

    // fetch_add returns the OLD value.
    std::atomic<int> c{5};
    assert(c.fetch_add(1) == 5 && c.load() == 6);

    // CAS loop: compare_exchange_weak may fail spuriously, so it lives in a loop.
    std::atomic<int> max_seen{0};
    auto update_max = [&](int candidate) {
        int current = max_seen.load(std::memory_order_relaxed);
        while (candidate > current &&
               !max_seen.compare_exchange_weak(current, candidate,
                                               std::memory_order_release,
                                               std::memory_order_relaxed)) {
            // on failure `current` has been refreshed with the actual value
        }
    };
    { std::jthread a{[&]{ update_max(10); }}, b{[&]{ update_max(7); }}; }
    assert(max_seen.load() == 10);
}

// -------------------------------------------- release/acquire publication
// The canonical lock-free publication pattern. Plain data + one atomic flag.
struct Frame {
    std::uint64_t seq{};
    double        payload[4]{};
};

static Frame            g_frame;
static std::atomic<bool> g_ready{false};

static void memory_ordering() {
    g_ready.store(false, std::memory_order_relaxed);

    std::jthread producer{[] {
        g_frame.seq = 42;                                  // (1) plain writes
        for (int i = 0; i < 4; ++i) g_frame.payload[i] = i * 1.5;
        // (2) RELEASE: everything written above is published to any thread that
        // performs an ACQUIRE load of g_ready and observes true.
        g_ready.store(true, std::memory_order_release);
    }};

    std::jthread consumer{[] {
        while (!g_ready.load(std::memory_order_acquire)) {  // (3) ACQUIRE
            std::this_thread::yield();
        }
        // (4) Guaranteed to see every write from (1). With relaxed on both sides
        // this could observe seq == 42 and payload still zero -- really, on ARM.
        assert(g_frame.seq == 42);
        assert(g_frame.payload[3] == 4.5);
    }};
    producer.join();
    consumer.join();

    // A standalone fence, for when the ordering cannot be attached to an access.
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

// -------------------------------------------------------------- false sharing
struct Unpadded {                     // two counters in ONE cache line
    std::atomic<std::uint64_t> a{0};
    std::atomic<std::uint64_t> b{0};
};
struct Padded {                       // one counter per cache line
    alignas(std::hardware_destructive_interference_size) std::atomic<std::uint64_t> a{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<std::uint64_t> b{0};
};

template <typename Counters>
static double hammer(int iterations) {
    Counters c;
    const auto start = std::chrono::steady_clock::now();
    {
        std::jthread t1{[&] { for (int i = 0; i < iterations; ++i)
                                  c.a.fetch_add(1, std::memory_order_relaxed); }};
        std::jthread t2{[&] { for (int i = 0; i < iterations; ++i)
                                  c.b.fetch_add(1, std::memory_order_relaxed); }};
    }
    const auto end = std::chrono::steady_clock::now();
    assert(c.a.load() == static_cast<std::uint64_t>(iterations));
    return std::chrono::duration<double, std::milli>(end - start).count();
}

static void false_sharing() {
    constexpr int kIters = 2'000'000;
    const double unpadded = hammer<Unpadded>(kIters);
    const double padded   = hammer<Padded>(kIters);
    std::cout << "  false sharing: unpadded " << unpadded << " ms vs padded "
              << padded << " ms  (ratio " << unpadded / padded << "x)\n";
    std::cout << "  hardware_destructive_interference_size = "
              << std::hardware_destructive_interference_size << " bytes\n";
    // The padded version is typically 2-10x faster. Same logic, same instruction
    // count -- the only difference is which cache line each counter lives in.
}

// ----------------------------------------------------- task-based parallelism
static void tasks() {
    // std::async: note the EXPLICIT launch policy. Without it the task may run
    // lazily on get(), and the returned future's destructor BLOCKS.
    auto fut = std::async(std::launch::async, [] { return 6 * 7; });
    assert(fut.get() == 42);

    // Exceptions propagate through the future.
    auto bad = std::async(std::launch::async, []() -> int { throw std::runtime_error{"x"}; });
    try { (void)bad.get(); assert(false); } catch (const std::runtime_error&) {}

    // promise/future as a one-shot channel.
    std::promise<int> p;
    auto f = p.get_future();
    { std::jthread setter{[&p] { p.set_value(7); }}; }
    assert(f.get() == 7);

    // C++20 latch: a one-shot "wait for N things".
    std::latch ready{3};
    std::atomic<int> arrived{0};
    {
        std::vector<std::jthread> ts;
        for (int i = 0; i < 3; ++i)
            ts.emplace_back([&] { arrived.fetch_add(1); ready.count_down(); });
        ready.wait();
        assert(arrived.load() == 3);
    }

    // C++20 barrier: a reusable rendezvous with a completion function.
    std::atomic<int> phases{0};
    std::barrier sync{2, [&] { phases.fetch_add(1); }};
    {
        auto phase_worker = [&] { for (int i = 0; i < 3; ++i) sync.arrive_and_wait(); };
        std::jthread x{phase_worker}, y{phase_worker};
    }
    assert(phases.load() == 3);
}

int main() {
    threads();
    mutexes();
    condition_variables();
    atomics();
    memory_ordering();
    false_sharing();
    tasks();
    std::cout << "section 11: all checks passed\n";
}
