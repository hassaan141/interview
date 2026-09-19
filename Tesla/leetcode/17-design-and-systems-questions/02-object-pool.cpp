// COMPONENT   A fixed-capacity object pool with an intrusive free list.
// WHERE       Frame buffers, message objects, tracker states -- anything allocated and
//             freed at a fixed rate in a real-time loop.
// WHY IT WINS malloc has NO BOUNDED WORST CASE: it may walk a free list, take a lock, or
//             call mmap. In a 100 Hz control loop that is disqualifying. A pool turns
//             allocation into "pop a pointer off a list" -- a handful of cycles, always.
//             It also eliminates fragmentation and keeps the objects contiguous.
//
// DESIGN DECISIONS:
//   - The free list is INTRUSIVE: an unused slot's own storage holds the index of the
//     next free slot, so the bookkeeping costs ZERO extra memory.
//   - Storage is raw aligned bytes plus PLACEMENT NEW, so objects are constructed on
//     acquire and destroyed on release -- the pool does not require T to be default
//     constructible, and no T exists in a free slot.
//   - acquire() returns an RAII HANDLE. A raw pointer would leak a slot on any early
//     return or exception; the handle cannot (course section 05).
//   - The handle is move-only: a pooled object has identity, not value semantics.
//   - Exhaustion returns an empty handle rather than throwing or growing: growing would
//     reintroduce the unbounded latency the pool exists to remove.
//
// COMPLEXITY  acquire and release are O(1), no allocation, no syscall, no lock.
//
// FOLLOW-UPS  Thread safe? -> a lock-free free list needs a CAS on the head and hits the
//             ABA problem; the standard fixes are a tagged pointer or per-thread pools
//             (which is what you actually want -- no contention at all).
//             Exhaustion policy? -> the caller decides: fail, drop the oldest, or block.
//             Make it explicit and COUNT IT; a silent failure is the bug.
//             Different sizes? -> a slab allocator: several pools by size class.
//             How do you know the capacity? -> Little's law (course section 14):
//             in-flight = rate x latency, plus headroom.

#include <array>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

template <typename T, std::size_t Capacity>
class ObjectPool {
    static_assert(Capacity > 0);
    static constexpr std::uint32_t kNone = 0xFFFFFFFFu;

    // Raw storage: no T is constructed until acquire().
    alignas(T) std::array<std::byte, Capacity * sizeof(T)> storage_{};
    std::array<std::uint32_t, Capacity> next_free_{};   // the intrusive free list
    std::uint32_t free_head_{0};
    std::uint32_t in_use_{0};
    std::uint32_t high_water_{0};                        // observability, not decoration

    T* slot(std::uint32_t i) noexcept {
        return reinterpret_cast<T*>(storage_.data() + static_cast<std::size_t>(i) * sizeof(T));
    }

public:
    // The RAII handle: releasing is not something a caller can forget.
    class Handle {
        ObjectPool* pool_{nullptr};
        std::uint32_t index_{kNone};
        friend class ObjectPool;
        Handle(ObjectPool* p, std::uint32_t i) noexcept : pool_{p}, index_{i} {}
    public:
        Handle() noexcept = default;
        ~Handle() { reset(); }
        Handle(const Handle&) = delete;                  // a slot has ONE owner
        Handle& operator=(const Handle&) = delete;
        Handle(Handle&& o) noexcept
            : pool_{std::exchange(o.pool_, nullptr)}, index_{std::exchange(o.index_, kNone)} {}
        Handle& operator=(Handle&& o) noexcept {
            if (this != &o) {
                reset();
                pool_ = std::exchange(o.pool_, nullptr);
                index_ = std::exchange(o.index_, kNone);
            }
            return *this;
        }
        void reset() noexcept {
            if (pool_) pool_->release(index_);
            pool_ = nullptr;
            index_ = kNone;
        }
        T* get() const noexcept { return pool_ ? pool_->slot(index_) : nullptr; }
        T& operator*()  const noexcept { return *get(); }
        T* operator->() const noexcept { return get(); }
        explicit operator bool() const noexcept { return pool_ != nullptr; }
    };

    ObjectPool() noexcept {
        for (std::uint32_t i = 0; i + 1 < Capacity; ++i) next_free_[i] = i + 1;
        next_free_[Capacity - 1] = kNone;
    }
    ~ObjectPool() { assert(in_use_ == 0 && "objects outlived their pool"); }
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    // Constructs a T in place. Returns an empty handle when exhausted -- it does NOT grow.
    template <typename... Args>
    [[nodiscard]] Handle acquire(Args&&... args) {
        if (free_head_ == kNone) return Handle{};        // exhausted: the caller decides
        const std::uint32_t index = free_head_;
        free_head_ = next_free_[index];
        ::new (static_cast<void*>(slot(index))) T(std::forward<Args>(args)...);
        ++in_use_;
        high_water_ = std::max(high_water_, in_use_);
        return Handle{this, index};
    }

    std::uint32_t in_use() const noexcept { return in_use_; }
    std::uint32_t high_water() const noexcept { return high_water_; }
    static constexpr std::size_t capacity() noexcept { return Capacity; }
    bool exhausted() const noexcept { return free_head_ == kNone; }

private:
    void release(std::uint32_t index) noexcept {
        slot(index)->~T();                                // destroy, then recycle the slot
        next_free_[index] = free_head_;
        free_head_ = index;
        --in_use_;
    }
};

// A test payload that counts construction and destruction, so the tests can prove the
// lifetimes are right rather than assuming it.
struct Tracked {
    static int live;
    static int constructed;
    int id;
    explicit Tracked(int i) : id{i} { ++live; ++constructed; }
    ~Tracked() { --live; }
    Tracked(const Tracked&) = delete;
    Tracked& operator=(const Tracked&) = delete;
};
int Tracked::live = 0;
int Tracked::constructed = 0;

int main() {
    ObjectPool<Tracked, 3> pool;
    assert(pool.in_use() == 0 && Tracked::live == 0);

    {
        auto a = pool.acquire(1);
        auto b = pool.acquire(2);
        assert(a && b && a->id == 1 && b->id == 2);
        assert(pool.in_use() == 2 && Tracked::live == 2);

        auto c = pool.acquire(3);
        assert(pool.exhausted());
        auto d = pool.acquire(4);                        // exhausted
        assert(!d && d.get() == nullptr);                // an EMPTY handle, not a crash
        assert(pool.in_use() == 3);

        // Moving transfers the slot; the source must not release it.
        auto moved = std::move(a);
        assert(!a && moved && moved->id == 1);
        assert(pool.in_use() == 3);                       // still three, not two
    }
    assert(pool.in_use() == 0 && Tracked::live == 0);      // every slot returned
    assert(pool.high_water() == 3);

    // Slots are genuinely reused: no allocation growth over many cycles.
    Tracked::constructed = 0;
    for (int i = 0; i < 10000; ++i) {
        auto h = pool.acquire(i);
        assert(h && h->id == i);
    }
    assert(Tracked::constructed == 10000 && Tracked::live == 0);
    assert(pool.in_use() == 0);

    // Explicit release via reset(), and reset() twice must be safe.
    auto h = pool.acquire(42);
    assert(pool.in_use() == 1);
    h.reset();
    assert(pool.in_use() == 0 && !h);
    h.reset();
    assert(pool.in_use() == 0);

    // Move assignment must release the handle's previous slot.
    auto x = pool.acquire(7);
    auto y = pool.acquire(8);
    x = std::move(y);
    assert(pool.in_use() == 1 && x->id == 8);
    return 0;
}
