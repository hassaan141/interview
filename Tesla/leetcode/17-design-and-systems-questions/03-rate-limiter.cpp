// COMPONENT   Rate limiters: a token bucket and a sliding-window counter.
// WHERE       Log throttling (so a fault does not flood the bus at 10 kHz), command rate
//             limits, back-pressure on a producer, retry budgets.
// WHY         "Allow at most N per second" has several inequivalent meanings, and naming
//             the difference is the interview:
//               FIXED WINDOW     -- simplest, but allows a 2x BURST across a boundary
//                                   (N at 0.999 s and N at 1.001 s).
//               SLIDING WINDOW LOG -- exact, but O(N) memory per key.
//               SLIDING WINDOW COUNTER -- interpolates the previous window; O(1) memory,
//                                   approximate, and what large systems actually use.
//               TOKEN BUCKET     -- O(1) memory AND allows a controlled burst of `capacity`
//                                   while enforcing a long-run rate. The right default.
//
// DESIGN DECISIONS:
//   - The CLOCK IS INJECTED as a template parameter, so the tests are deterministic with
//     no sleeps (course section 13). A rate limiter tested with sleep() is a flaky test.
//   - Tokens are refilled LAZILY on each query rather than by a timer thread: no thread,
//     no wakeups, and the arithmetic is exact.
//   - Refill uses integer arithmetic scaled by the period, not floating point, so there is
//     no drift and no rounding surprise (course section 01).
//
// COMPLEXITY  try_acquire is O(1), no allocation, no lock (add one if shared).
//
// FOLLOW-UPS  Distributed? -> the state has to move to a shared store; then you are
//             trading exactness for a round trip, and the usual answer is a local bucket
//             plus periodic reconciliation.
//             Per-key limits? -> a hash map of buckets, plus an eviction policy (an LRU,
//             see section 06) or you leak memory on unbounded keys.
//             Thread safe? -> a mutex is fine at these rates; a lock-free version needs a
//             CAS loop on a packed {tokens, timestamp} word.
//             Fairness across callers? -> a token bucket is not fair; add a queue if you
//             need FIFO service.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>

// A deterministic fake clock in nanoseconds. Production would use steady_clock.
struct FakeClock {
    static std::int64_t now_ns;
    static std::int64_t now() noexcept { return now_ns; }
    static void advance_ms(std::int64_t ms) { now_ns += ms * 1'000'000; }
};
std::int64_t FakeClock::now_ns = 0;

// ---------------------------------------------------------------- token bucket
template <typename Clock>
class TokenBucket {
    std::int64_t capacity_;          // maximum burst
    std::int64_t refill_per_sec_;    // long-run rate
    std::int64_t tokens_;            // scaled by kScale to avoid floating point
    std::int64_t last_ns_;
    static constexpr std::int64_t kScale = 1'000'000;   // sub-token resolution

public:
    TokenBucket(std::int64_t capacity, std::int64_t refill_per_sec)
        : capacity_{capacity}, refill_per_sec_{refill_per_sec},
          tokens_{capacity * kScale}, last_ns_{Clock::now()} {}

    [[nodiscard]] bool try_acquire(std::int64_t count = 1) noexcept {
        refill();
        const std::int64_t needed = count * kScale;
        if (tokens_ < needed) return false;
        tokens_ -= needed;
        return true;
    }
    std::int64_t available() noexcept { refill(); return tokens_ / kScale; }

private:
    void refill() noexcept {
        const std::int64_t now = Clock::now();
        const std::int64_t elapsed_ns = now - last_ns_;
        if (elapsed_ns <= 0) return;
        last_ns_ = now;
        // Integer arithmetic: tokens per ns = refill_per_sec / 1e9, scaled by kScale.
        const std::int64_t gained = elapsed_ns * refill_per_sec_ * kScale / 1'000'000'000;
        tokens_ = std::min(capacity_ * kScale, tokens_ + gained);   // clamp to the burst
    }
};

// ------------------------------------------------- sliding window LOG (exact, O(N))
template <typename Clock>
class SlidingWindowLog {
    std::int64_t window_ns_;
    std::size_t limit_;
    std::deque<std::int64_t> timestamps_;      // O(N) memory -- the cost of exactness
public:
    SlidingWindowLog(std::size_t limit, std::int64_t window_ms)
        : window_ns_{window_ms * 1'000'000}, limit_{limit} {}

    [[nodiscard]] bool try_acquire() {
        const std::int64_t now = Clock::now();
        while (!timestamps_.empty() && timestamps_.front() <= now - window_ns_)
            timestamps_.pop_front();                    // evict anything outside the window
        if (timestamps_.size() >= limit_) return false;
        timestamps_.push_back(now);
        return true;
    }
    std::size_t used() const noexcept { return timestamps_.size(); }
};

// ------------------------------------- fixed window, shown to demonstrate its FLAW
template <typename Clock>
class FixedWindowCounter {
    std::int64_t window_ns_;
    std::size_t limit_;
    std::int64_t window_start_{0};
    std::size_t count_{0};
public:
    FixedWindowCounter(std::size_t limit, std::int64_t window_ms)
        : window_ns_{window_ms * 1'000'000}, limit_{limit}, window_start_{Clock::now()} {}
    [[nodiscard]] bool try_acquire() noexcept {
        const std::int64_t now = Clock::now();
        if (now - window_start_ >= window_ns_) { window_start_ = now; count_ = 0; }
        if (count_ >= limit_) return false;
        ++count_;
        return true;
    }
};

int main() {
    // ---- token bucket: burst up to capacity, then the sustained rate ------------
    FakeClock::now_ns = 0;
    TokenBucket<FakeClock> bucket{/*capacity=*/5, /*refill_per_sec=*/10};

    for (int i = 0; i < 5; ++i) assert(bucket.try_acquire());   // the full burst
    assert(!bucket.try_acquire());                               // bucket empty

    FakeClock::advance_ms(100);                                  // 10/s -> 1 token
    assert(bucket.try_acquire());
    assert(!bucket.try_acquire());

    FakeClock::advance_ms(1000);                                 // would be 10 tokens...
    assert(bucket.available() == 5);                             // ...but CLAMPED to the burst
    for (int i = 0; i < 5; ++i) assert(bucket.try_acquire());
    assert(!bucket.try_acquire());

    // Acquiring several at once, and asking for more than the capacity.
    FakeClock::advance_ms(1000);
    assert(bucket.try_acquire(3));
    assert(!bucket.try_acquire(3));                              // only 2 left
    assert(bucket.try_acquire(2));
    assert(!bucket.try_acquire(100));                            // never satisfiable

    // ---- sliding window log: exact, and it slides ------------------------------
    FakeClock::now_ns = 0;
    SlidingWindowLog<FakeClock> log{/*limit=*/3, /*window_ms=*/1000};
    assert(log.try_acquire() && log.try_acquire() && log.try_acquire());
    assert(!log.try_acquire());
    FakeClock::advance_ms(500);
    assert(!log.try_acquire());                                  // all three still in window
    FakeClock::advance_ms(501);                                  // the first three aged out
    assert(log.try_acquire() && log.try_acquire() && log.try_acquire());
    assert(!log.try_acquire());

    // ---- fixed window: demonstrate the 2x BOUNDARY BURST -----------------------
    FakeClock::now_ns = 0;
    FixedWindowCounter<FakeClock> fixed{/*limit=*/3, /*window_ms=*/1000};
    FakeClock::advance_ms(999);
    assert(fixed.try_acquire() && fixed.try_acquire() && fixed.try_acquire());
    FakeClock::advance_ms(2);                                    // a new window opens
    assert(fixed.try_acquire() && fixed.try_acquire() && fixed.try_acquire());
    // 6 permits in ~3 ms with a "3 per second" limit. THAT is why fixed windows are
    // not used for anything that matters.

    // The sliding log does not have that flaw, on the same timeline.
    FakeClock::now_ns = 0;
    SlidingWindowLog<FakeClock> strict{3, 1000};
    FakeClock::advance_ms(999);
    assert(strict.try_acquire() && strict.try_acquire() && strict.try_acquire());
    FakeClock::advance_ms(2);
    assert(!strict.try_acquire());                               // correctly refused
    return 0;
}
