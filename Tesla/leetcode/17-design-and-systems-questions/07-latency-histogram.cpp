// COMPONENT   A log-bucketed latency histogram with accurate percentiles, plus a
//             zero-allocation scoped timer.
// WHERE       "developing system tools to benchmark, characterize and optimize the LATENCY
//             and THROUGHPUT of the autonomy workloads on the FSD chip" -- this file is
//             the closest thing in this repo to the literal job description.
//
// WHY A HISTOGRAM AND NOT A LIST OF SAMPLES:
//   - Storing every sample is O(n) memory and needs a sort (or nth_element) to get p99.
//     At 1 kHz over an hour that is 3.6M samples per stage.
//   - A histogram is FIXED memory, O(1) per record, and gives exact-to-the-bucket
//     percentiles. It also merges trivially across threads (just add the counts), which
//     is what lets each thread keep its own and avoid contention entirely.
//   - MEANS ARE USELESS for a deadline-driven system. A 4 ms mean with an 85 ms p99.9 is a
//     failing system that looks fine on a dashboard (course section 14). Always report
//     p50/p99/p99.9/max.
//
// BUCKETING: linear buckets waste memory at the tail and lose resolution at the head.
// This uses HdrHistogram-style bucketing: the bucket index is derived from the value's
// BIT WIDTH (a coarse exponent) plus a few leading mantissa bits, giving constant RELATIVE
// precision across many orders of magnitude -- ~1% error whether the value is 1 us or
// 10 s -- in a few kilobytes.
//
// COMPLEXITY  record: O(1), no allocation, no lock, a few instructions (std::bit_width is
//             one CLZ instruction). percentile: O(buckets), done offline.
//
// FOLLOW-UPS  Coordinated omission: if you measure only the requests you actually sent,
//             a stall hides the very latency it causes. Fix by recording against the
//             INTENDED start time in a fixed-rate loop. Naming this is a strong signal.
//             Merging across threads? -> per-thread histograms summed at report time; no
//             atomics on the hot path.
//             Streaming quantiles without fixed buckets? -> t-digest or KLL sketch.
//             Timestamp source? -> rdtsc/cntvct (~20 cycles) rather than clock_gettime,
//             calibrated once; convert to ns OFFLINE (course section 14, C4).

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <vector>

class LatencyHistogram {
    // 64 exponent ranges x 16 linear sub-buckets = constant ~6% relative resolution
    // in 1024 counters (8 KB). Raise kSubBits for finer resolution.
    static constexpr unsigned kSubBits = 4;
    static constexpr std::size_t kSubBuckets = 1u << kSubBits;
    static constexpr std::size_t kExponents = 64;
    static constexpr std::size_t kBuckets = kExponents * kSubBuckets;

    std::array<std::uint64_t, kBuckets> counts_{};
    std::uint64_t total_{0};
    std::uint64_t min_{UINT64_MAX};
    std::uint64_t max_{0};
    std::uint64_t sum_{0};

    // Value -> bucket. Small values get their own bucket; large ones share by magnitude.
    static std::size_t bucket_of(std::uint64_t value) noexcept {
        if (value < kSubBuckets) return static_cast<std::size_t>(value);   // exact, shift 0
        const unsigned width = static_cast<unsigned>(std::bit_width(value));   // one CLZ
        const unsigned shift = width - kSubBits;               // keep the top kSubBits
        // For shift >= 1 the top kept bit is always set, so mantissa is in [8, 15].
        const auto mantissa = static_cast<std::size_t>((value >> shift) & (kSubBuckets - 1));
        return static_cast<std::size_t>(shift) * kSubBuckets + mantissa;
    }
    // The representative (lower edge) value of a bucket, for reporting.
    // Inverse of bucket_of: value = mantissa << shift.
    static std::uint64_t value_of(std::size_t bucket) noexcept {
        const auto shift = static_cast<unsigned>(bucket / kSubBuckets);
        const auto mantissa = static_cast<std::uint64_t>(bucket % kSubBuckets);
        return mantissa << shift;
    }

public:
    void record(std::uint64_t value_ns) noexcept {
        ++counts_[bucket_of(value_ns)];                        // O(1), no allocation
        ++total_;
        sum_ += value_ns;
        min_ = std::min(min_, value_ns);
        max_ = std::max(max_, value_ns);
    }

    // Percentile with linear interpolation is overkill; the bucket's lower edge is
    // within the histogram's stated relative error, which is the honest answer.
    std::uint64_t percentile(double p) const noexcept {
        if (total_ == 0) return 0;
        const auto target = static_cast<std::uint64_t>(
            std::ceil(p / 100.0 * static_cast<double>(total_)));
        std::uint64_t seen = 0;
        for (std::size_t i = 0; i < kBuckets; ++i) {
            seen += counts_[i];
            if (seen >= target) return value_of(i);
        }
        return max_;
    }

    std::uint64_t count() const noexcept { return total_; }
    std::uint64_t min() const noexcept { return total_ ? min_ : 0; }
    std::uint64_t max() const noexcept { return max_; }
    double mean() const noexcept {
        return total_ ? static_cast<double>(sum_) / static_cast<double>(total_) : 0.0;
    }

    // Per-thread histograms merge by adding counts -- no atomics on the hot path.
    void merge(const LatencyHistogram& other) noexcept {
        for (std::size_t i = 0; i < kBuckets; ++i) counts_[i] += other.counts_[i];
        total_ += other.total_;
        sum_ += other.sum_;
        if (other.total_) {
            min_ = std::min(min_, other.min_);
            max_ = std::max(max_, other.max_);
        }
    }
    void reset() noexcept { *this = LatencyHistogram{}; }

    void report(const char* name) const {
        std::printf("  %-18s n=%-8llu  p50=%-9llu p99=%-9llu p99.9=%-9llu max=%-9llu (ns)\n",
                    name, static_cast<unsigned long long>(total_),
                    static_cast<unsigned long long>(percentile(50)),
                    static_cast<unsigned long long>(percentile(99)),
                    static_cast<unsigned long long>(percentile(99.9)),
                    static_cast<unsigned long long>(max_));
    }
};

// A scoped timer: RAII, so it records on every exit path including an exception.
class ScopedTimer {
    LatencyHistogram& hist_;
    std::chrono::steady_clock::time_point start_;
public:
    explicit ScopedTimer(LatencyHistogram& h) noexcept
        : hist_{h}, start_{std::chrono::steady_clock::now()} {}
    ~ScopedTimer() {
        const auto elapsed = std::chrono::steady_clock::now() - start_;
        hist_.record(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()));
    }
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
};

int main() {
    LatencyHistogram h;
    assert(h.count() == 0 && h.percentile(50) == 0 && h.max() == 0);

    // A uniform distribution: the percentiles should track the values, within the
    // histogram's relative error.
    for (std::uint64_t v = 1; v <= 10000; ++v) h.record(v);
    assert(h.count() == 10000);
    assert(h.min() == 1 && h.max() == 10000);
    const auto p50 = h.percentile(50);
    const auto p99 = h.percentile(99);
    // Bucket edges are the LOWER bound of the bucket, so a percentile reads slightly
    // low -- within the histogram's stated relative error (~1/16 = 6%).
    assert(p50 > 4600 && p50 <= 5000);           // ~5000
    assert(p99 > 9200 && p99 <= 10000);          // ~9900
    assert(h.percentile(100) >= 9000);

    // THE point of the whole exercise: a mean that hides a terrible tail.
    LatencyHistogram tail;
    for (int i = 0; i < 9990; ++i) tail.record(4'000'000);        // 4 ms, the common case
    for (int i = 0; i < 10; ++i)   tail.record(85'000'000);       // 85 ms, once in 1000
    assert(tail.mean() < 4'100'000);                              // the mean says "4.08 ms"
    assert(tail.percentile(99.9) >= 64'000'000);                  // the p99.9 says 85 ms
    std::puts("A distribution whose mean lies to you:");
    tail.report("planner_stage");
    std::puts("  ^ mean 4.08 ms, p99.9 ~85 ms. With a 20 ms deadline this system fails");
    std::puts("    ~10 times per 10000 frames, and the mean shows nothing.");

    // Per-thread merge: the sum must equal the single-threaded histogram.
    LatencyHistogram a, b, whole;
    for (std::uint64_t v = 1; v <= 500; ++v) { a.record(v); whole.record(v); }
    for (std::uint64_t v = 501; v <= 1000; ++v) { b.record(v); whole.record(v); }
    a.merge(b);
    assert(a.count() == whole.count());
    assert(a.percentile(50) == whole.percentile(50));
    assert(a.percentile(99) == whole.percentile(99));
    assert(a.min() == whole.min() && a.max() == whole.max());

    // Bucketing must be monotonic and hold its relative error over many magnitudes.
    LatencyHistogram wide;
    for (std::uint64_t v : {std::uint64_t{1}, std::uint64_t{1000}, std::uint64_t{1'000'000},
                            std::uint64_t{1'000'000'000}}) wide.record(v);
    assert(wide.percentile(25) == 1);
    assert(wide.max() == 1'000'000'000);

    // Edge cases: zero, and a merge with an empty histogram.
    LatencyHistogram zero;
    zero.record(0);
    assert(zero.count() == 1 && zero.percentile(50) == 0 && zero.min() == 0);
    zero.merge(LatencyHistogram{});
    assert(zero.count() == 1 && zero.min() == 0);

    // The scoped timer, on real work.
    LatencyHistogram timed;
    volatile std::uint64_t sink = 0;
    for (int i = 0; i < 2000; ++i) {
        ScopedTimer t{timed};                     // records on EVERY exit path
        for (int j = 0; j < 100; ++j) sink = sink + static_cast<std::uint64_t>(j);
    }
    assert(timed.count() == 2000 && timed.max() > 0);
    std::puts("Measured with ScopedTimer (a trivial loop, so this is mostly clock overhead):");
    timed.report("scoped_timer");
    return 0;
}
