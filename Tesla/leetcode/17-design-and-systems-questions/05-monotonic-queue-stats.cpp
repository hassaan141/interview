// COMPONENT   (a) A queue with O(1) amortized min and max.  (b) Streaming statistics
//             (count/mean/variance) with no stored samples.
// WHERE       Rolling sensor statistics, outlier rejection, a moving max for a watchdog,
//             online calibration -- anywhere you process a stream at a fixed rate and
//             cannot afford to store or re-scan a window.
//
// (a) MONOTONIC DEQUE. A heap gives O(log n) push/pop and CANNOT remove an arbitrary
//     expired element. A deque of candidates kept in decreasing order gives O(1)
//     amortized: when a new value arrives, every smaller value behind it can never be the
//     max again (the new one is larger AND stays in the window longer), so pop them.
//     Each element is pushed once and popped once -> amortized O(1).
//
// (b) WELFORD'S ALGORITHM. The naive variance E[x^2] - E[x]^2 subtracts two large nearly
//     equal numbers and can even go NEGATIVE (course section 01). Welford updates the mean
//     and the sum of squared deviations incrementally with no cancellation, in O(1) time
//     and O(1) memory. This is the single most useful numerical recipe for telemetry.
//
// COMPLEXITY  push/pop/min/max: O(1) amortized, O(window) memory. Stats: O(1) both.
//
// FOLLOW-UPS  Sliding window MEDIAN? -> two heaps do not support arbitrary erase; use an
//             ordered multiset with an iterator to the middle, or lazy deletion --
//             O(log n). Say that the deque trick does NOT generalize to the median.
//             Fixed-size window? -> the same deque, evicting by index (see
//             leetcode/03/06-sliding-window-maximum.cpp).
//             Merging statistics from several threads? -> Welford has a parallel merge
//             form (Chan's algorithm), which is how you combine per-thread accumulators.
//             Robust to outliers? -> a median or a trimmed mean, not the mean.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <cstddef>
#include <deque>
#include <optional>
#include <vector>

// ---------------------------------------------------- queue with O(1) min and max
template <typename T>
class MinMaxQueue {
    std::deque<T> values_;
    std::deque<T> max_candidates_;      // DECREASING: front is the maximum
    std::deque<T> min_candidates_;      // INCREASING: front is the minimum
public:
    void push(const T& v) {
        values_.push_back(v);
        // Anything smaller behind us can never be the max again.
        while (!max_candidates_.empty() && max_candidates_.back() < v) max_candidates_.pop_back();
        max_candidates_.push_back(v);
        while (!min_candidates_.empty() && min_candidates_.back() > v) min_candidates_.pop_back();
        min_candidates_.push_back(v);
    }
    void pop() {
        assert(!values_.empty());
        const T v = values_.front();
        values_.pop_front();
        // Only remove it from a candidate list if it was actually the extreme.
        if (!max_candidates_.empty() && max_candidates_.front() == v) max_candidates_.pop_front();
        if (!min_candidates_.empty() && min_candidates_.front() == v) min_candidates_.pop_front();
    }
    std::optional<T> max() const {
        return max_candidates_.empty() ? std::nullopt : std::optional<T>{max_candidates_.front()};
    }
    std::optional<T> min() const {
        return min_candidates_.empty() ? std::nullopt : std::optional<T>{min_candidates_.front()};
    }
    std::optional<T> front() const {
        return values_.empty() ? std::nullopt : std::optional<T>{values_.front()};
    }
    std::size_t size() const noexcept { return values_.size(); }
    bool empty() const noexcept { return values_.empty(); }
};

// ----------------------------------------- streaming statistics (Welford + min/max)
class StreamingStats {
    std::size_t count_{0};
    double mean_{0.0};
    double m2_{0.0};                    // the sum of squared deviations
    double min_{0.0}, max_{0.0};
public:
    void add(double x) {
        ++count_;
        if (count_ == 1) { min_ = max_ = x; }
        else { min_ = std::min(min_, x); max_ = std::max(max_, x); }
        // Welford: no E[x^2] - E[x]^2, so no catastrophic cancellation.
        const double delta = x - mean_;
        mean_ += delta / static_cast<double>(count_);
        m2_ += delta * (x - mean_);      // NOTE: the UPDATED mean
    }
    std::size_t count() const noexcept { return count_; }
    double mean() const noexcept { return count_ ? mean_ : 0.0; }
    double variance() const noexcept { return count_ ? m2_ / static_cast<double>(count_) : 0.0; }
    double sample_variance() const noexcept {
        return count_ > 1 ? m2_ / static_cast<double>(count_ - 1) : 0.0;
    }
    double stddev() const noexcept { return std::sqrt(variance()); }
    double min() const noexcept { return min_; }
    double max() const noexcept { return max_; }

    // Chan's parallel merge: combine two independently computed accumulators.
    void merge(const StreamingStats& other) {
        if (other.count_ == 0) return;
        if (count_ == 0) { *this = other; return; }
        const double n_a = static_cast<double>(count_), n_b = static_cast<double>(other.count_);
        const double delta = other.mean_ - mean_;
        const double total = n_a + n_b;
        m2_ += other.m2_ + delta * delta * n_a * n_b / total;
        mean_ += delta * n_b / total;
        count_ += other.count_;
        min_ = std::min(min_, other.min_);
        max_ = std::max(max_, other.max_);
    }
};

static bool near(double a, double b, double tol = 1e-9) { return std::fabs(a - b) < tol; }

int main() {
    // ---- MinMaxQueue ----------------------------------------------------------
    MinMaxQueue<int> q;
    assert(q.empty() && !q.min().has_value() && !q.max().has_value());

    for (int v : {3, 1, 4, 1, 5}) q.push(v);
    assert(q.size() == 5 && q.min() == 1 && q.max() == 5);
    q.pop();                                      // removes 3
    assert(q.min() == 1 && q.max() == 5);
    q.pop();                                      // removes 1, but another 1 remains
    assert(q.min() == 1 && q.max() == 5);         // the duplicate must survive
    q.pop();                                      // removes 4
    assert(q.min() == 1 && q.max() == 5);
    q.pop();                                      // removes the second 1
    assert(q.min() == 5 && q.max() == 5);
    q.pop();
    assert(q.empty() && !q.min().has_value());

    // A monotonically decreasing sequence exercises the "pop everything" path.
    MinMaxQueue<int> d;
    for (int v : {5, 4, 3, 2, 1}) d.push(v);
    assert(d.max() == 5 && d.min() == 1);
    d.pop();
    assert(d.max() == 4);                          // the max left the window

    // Model-based check against a brute-force scan.
    {
        MinMaxQueue<int> m;
        std::vector<int> model;
        std::uint32_t rng = 99;
        for (int i = 0; i < 5000; ++i) {
            rng = rng * 1103515245u + 12345u;
            if (model.empty() || ((rng >> 16) & 3)) {
                const int v = static_cast<int>(rng % 100);
                m.push(v);
                model.push_back(v);
            } else {
                m.pop();
                model.erase(model.begin());
            }
            if (model.empty()) { assert(m.empty()); continue; }
            int lo = model[0], hi = model[0];
            for (int v : model) { lo = std::min(lo, v); hi = std::max(hi, v); }
            assert(m.min() == lo && m.max() == hi);
        }
    }

    // ---- StreamingStats -------------------------------------------------------
    StreamingStats s;
    assert(s.count() == 0 && near(s.mean(), 0.0));
    for (double x : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) s.add(x);
    assert(s.count() == 8);
    assert(near(s.mean(), 5.0));
    assert(near(s.variance(), 4.0));               // population variance
    assert(near(s.stddev(), 2.0));
    assert(near(s.min(), 2.0) && near(s.max(), 9.0));

    // THE case that breaks the naive formula: large values with a small spread.
    StreamingStats big;
    for (double x : {1e8, 1e8 + 1.0, 1e8 + 2.0}) big.add(x);
    assert(near(big.variance(), 2.0 / 3.0, 1e-6));  // the naive version returns garbage

    // Parallel merge must equal the sequential result.
    StreamingStats a, b, whole;
    for (double x : {1.0, 2.0, 3.0, 4.0}) { a.add(x); whole.add(x); }
    for (double x : {10.0, 20.0, 30.0}) { b.add(x); whole.add(x); }
    a.merge(b);
    assert(a.count() == whole.count());
    assert(near(a.mean(), whole.mean(), 1e-9));
    assert(near(a.variance(), whole.variance(), 1e-9));

    StreamingStats empty_merge;
    empty_merge.merge(StreamingStats{});
    assert(empty_merge.count() == 0);
    return 0;
}
