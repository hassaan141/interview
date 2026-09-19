// 14 — Performance: a real benchmark harness plus the experiments that matter.
//
//   g++ -std=c++20 -O2 -march=native -g -fno-omit-frame-pointer examples.cpp -o ex
//   taskset -c 2 ./ex
//
// Then look at WHY with perf:
//   perf stat -e cycles,instructions,cache-misses,branch-misses ./ex
//   perf record -g ./ex && perf report
//
// Every number below is measured on the machine you run it on. Do not trust the
// numbers in a blog post (including mine) -- reproduce them.

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

// ===================================================== the harness itself
// Rule 2: stop the optimizer from deleting the work being measured.
template <typename T>
static inline void do_not_optimize(T const& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}
static inline void clobber_memory() { asm volatile("" : : : "memory"); }

struct Stats { double p50, p99, mean, min; };

// Rule 3/4: warm up, then collect a DISTRIBUTION and report percentiles.
template <typename F>
static Stats bench(F&& fn, int reps = 25, int inner = 1) {
    for (int i = 0; i < std::max(3, reps / 5); ++i) fn();        // warm up
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(reps));
    for (int r = 0; r < reps; ++r) {
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < inner; ++i) fn();
        const auto t1 = std::chrono::steady_clock::now();
        samples.push_back(
            std::chrono::duration<double, std::nano>(t1 - t0).count() / inner);
    }
    std::ranges::sort(samples);
    const double mean = std::accumulate(samples.begin(), samples.end(), 0.0)
                        / static_cast<double>(samples.size());
    const auto idx = [&](double q) {
        return samples[std::min(samples.size() - 1,
                                static_cast<std::size_t>(q * static_cast<double>(samples.size())))];
    };
    return {idx(0.50), idx(0.99), mean, samples.front()};
}

static void row(const char* name, const Stats& s, double baseline_p50 = 0.0) {
    std::printf("    %-34s p50 %9.1f ns   p99 %9.1f ns", name, s.p50, s.p99);
    if (baseline_p50 > 0.0) {
        const double r = baseline_p50 / s.p50;
        if (r >= 1.0) std::printf("   %6.2fx faster", r);
        else          std::printf("   %6.2fx SLOWER", 1.0 / r);
    }
    std::printf("\n");
}

// ============================================ 1. sequential vs. random access
// The single most important performance fact in this file: the SAME number of
// operations, differing only in access order.
static void access_pattern() {
    constexpr std::size_t kN = 1 << 22;              // 16 MB of ints: bigger than L2/L3
    std::vector<std::uint32_t> data(kN);
    std::iota(data.begin(), data.end(), 0u);

    // A permutation of the indices: same count of loads, unpredictable addresses.
    std::vector<std::uint32_t> shuffled(kN);
    std::iota(shuffled.begin(), shuffled.end(), 0u);
    std::mt19937 gen{42};
    std::ranges::shuffle(shuffled, gen);

    std::puts("  1. access pattern (same work, different order)");
    const auto seq = bench([&] {
        std::uint64_t sum = 0;
        for (std::size_t i = 0; i < kN; ++i) sum += data[i];        // stride 1
        do_not_optimize(sum);
    }, 15);
    const auto rnd = bench([&] {
        std::uint64_t sum = 0;
        for (std::size_t i = 0; i < kN; ++i) sum += data[shuffled[i]];   // random
        do_not_optimize(sum);
    }, 15);
    row("sequential", seq);
    row("random (pointer-chase-like)", rnd, seq.p50);
    std::puts("    ^ the prefetcher predicts stride-1 perfectly and cannot predict");
    std::puts("      a permutation. Layout, not instruction count, is the lever.");
}

// ================================================= 2. AoS vs. SoA
struct DetectionAoS {                        // 20 bytes -> padded to 20/24
    float x, y, z, conf;
    std::uint32_t id;
};
struct DetectionsSoA {
    std::vector<float> x, y, z, conf;
    std::vector<std::uint32_t> id;
};

static void aos_vs_soa() {
    constexpr std::size_t kN = 1 << 21;
    std::vector<DetectionAoS> aos(kN);
    DetectionsSoA soa;
    soa.x.resize(kN); soa.y.resize(kN); soa.z.resize(kN);
    soa.conf.resize(kN); soa.id.resize(kN);

    std::mt19937 gen{7};
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};
    for (std::size_t i = 0; i < kN; ++i) {
        const float c = dist(gen);
        aos[i] = {dist(gen), dist(gen), dist(gen), c, static_cast<std::uint32_t>(i)};
        soa.conf[i] = c;
    }

    std::puts("  2. AoS vs SoA for a pass that reads ONE field");
    const auto a = bench([&] {                      // touches 20 B per element
        std::size_t n = 0;
        for (const auto& d : aos) n += (d.conf > 0.5f);
        do_not_optimize(n);
    }, 20);
    const auto s = bench([&] {                      // touches 4 B per element
        std::size_t n = 0;
        for (float c : soa.conf) n += (c > 0.5f);
        do_not_optimize(n);
    }, 20);
    row("AoS (reads 20 B/element)", a);
    row("SoA (reads 4 B/element)", s, a.p50);
    std::printf("    sizeof(DetectionAoS) = %zu -> a 64 B cache line holds %.1f\n",
                sizeof(DetectionAoS), 64.0 / static_cast<double>(sizeof(DetectionAoS)));
    std::puts("    ^ SoA moves 5x less memory for this pass, and vectorizes cleanly.");
    std::puts("      AoS wins when a pass touches MOST fields of one element.");
}

// ================================================= 3. struct layout / padding
struct BadOrder  { char a; double b; char c; std::int32_t d; };   // padding everywhere
struct GoodOrder { double b; std::int32_t d; char a; char c; };   // packed

static void struct_layout() {
    std::puts("  3. member ordering");
    std::printf("    sizeof(BadOrder)  = %zu  (declared char,double,char,int32)\n",
                sizeof(BadOrder));
    std::printf("    sizeof(GoodOrder) = %zu  (declared double,int32,char,char)\n",
                sizeof(GoodOrder));
    static_assert(sizeof(BadOrder) == 24 && sizeof(GoodOrder) == 16);
    std::puts("    ^ 33% less memory for an array of them, for free. -Wpadded finds it.");
}

// ============================================ 4. branch prediction
static void branch_prediction() {
    constexpr std::size_t kN = 1 << 20;
    std::vector<int> random_data(kN), sorted_data(kN);
    std::mt19937 gen{3};
    std::uniform_int_distribution<int> dist{0, 255};
    for (auto& v : random_data) v = dist(gen);
    sorted_data = random_data;
    std::ranges::sort(sorted_data);

    // A COUNT can be made branchless by the compiler (cmov / a vectorized compare),
    // which hides the effect. A FILTER cannot: the store is conditional, so the
    // branch really is taken or not taken, and the predictor's accuracy shows up.
    std::vector<int> out;
    out.reserve(kN);
    auto filter = [&out](const std::vector<int>& v) {
        out.clear();
        for (int x : v) if (x > 128) out.push_back(x);   // genuinely conditional
        return out.size();
    };
    auto count_branchless = [](const std::vector<int>& v) {
        std::uint64_t n = 0;
        for (int x : v) n += static_cast<unsigned>(x > 128);   // no branch at all
        return n;
    };

    std::puts("  4. branch prediction (identical work, identical results)");
    const auto f_rand   = bench([&] { do_not_optimize(filter(random_data)); }, 20);
    const auto f_sorted = bench([&] { do_not_optimize(filter(sorted_data)); }, 20);
    const auto bl_rand  = bench([&] { do_not_optimize(count_branchless(random_data)); }, 20);
    row("filter, RANDOM input", f_rand);
    row("filter, SORTED input", f_sorted, f_rand.p50);
    row("branchless count, random input", bl_rand, f_rand.p50);
    std::puts("    ^ sorting makes the branch predictable: same instructions, same");
    std::puts("      output, fewer mispredicts (~15-20 cycles each). The branchless");
    std::puts("      COUNT is faster still because there is no branch to mispredict --");
    std::puts("      note it does less work, so compare it only as an upper bound.");
    std::puts("      Verify with: perf stat -e branches,branch-misses");
}

// ============================================ 5. dependency chains / ILP
static void dependency_chains() {
    constexpr std::size_t kN = 1 << 20;
    std::vector<float> v(kN, 1.000001f);

    std::puts("  5. dependency chains (one accumulator vs four)");
    const auto serial = bench([&] {
        float s = 0.0f;
        for (std::size_t i = 0; i < kN; ++i) s += v[i];     // each add waits for the last
        do_not_optimize(s);
    }, 20);
    const auto unrolled = bench([&] {
        float s0 = 0, s1 = 0, s2 = 0, s3 = 0;
        std::size_t i = 0;
        for (; i + 3 < kN; i += 4) {                        // four independent chains
            s0 += v[i]; s1 += v[i + 1]; s2 += v[i + 2]; s3 += v[i + 3];
        }
        for (; i < kN; ++i) s0 += v[i];
        const float s = (s0 + s1) + (s2 + s3);
        do_not_optimize(s);
    }, 20);
    row("1 accumulator (serial chain)", serial);
    row("4 accumulators (ILP)", unrolled, serial.p50);
    std::puts("    ^ FP add has ~4 cycle latency but 1-2/cycle throughput, so a single");
    std::puts("      chain leaves the pipeline idle. NOTE: the two results DIFFER in");
    std::puts("      the last bits -- FP addition is not associative (section 01).");
    std::puts("      That is exactly why -ffast-math can do this and -O2 alone cannot.");
}

// ============================================ 6. allocation in the hot path
static void allocation_cost() {
    constexpr std::size_t kInner = 512;
    std::puts("  6. allocating in the hot path vs. reusing a buffer");

    const auto allocating = bench([&] {
        std::vector<float> tmp(kInner);               // malloc + free EVERY call
        for (std::size_t i = 0; i < kInner; ++i) tmp[i] = static_cast<float>(i);
        do_not_optimize(tmp[0]);
    }, 30, 200);

    std::vector<float> reused(kInner);                 // allocated once
    const auto reusing = bench([&] {
        for (std::size_t i = 0; i < kInner; ++i) reused[i] = static_cast<float>(i);
        do_not_optimize(reused[0]);
    }, 30, 200);

    row("vector per iteration", allocating);
    row("reused buffer", reusing, allocating.p50);
    std::puts("    ^ this is the #1 real-time rule: no allocation in the hot path.");
    std::puts("      malloc is not just slow, it has NO bounded worst case.");
}

// ============================================ 7. aliasing blocks vectorization
// Without __restrict the compiler must assume out may overlap a or b, so it cannot
// vectorize: it must re-load after every store.
static void add_maybe_aliased(float* out, const float* a, const float* b, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) out[i] = a[i] + b[i];
}
static void add_restrict(float* __restrict out, const float* __restrict a,
                         const float* __restrict b, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) out[i] = a[i] + b[i];
}

static void aliasing() {
    constexpr std::size_t kN = 1 << 16;
    std::vector<float> a(kN, 1.0f), b(kN, 2.0f), out(kN);

    std::puts("  7. pointer aliasing and vectorization");
    const auto plain = bench([&] {
        add_maybe_aliased(out.data(), a.data(), b.data(), kN);
        clobber_memory();
    }, 30, 20);
    const auto restr = bench([&] {
        add_restrict(out.data(), a.data(), b.data(), kN);
        clobber_memory();
    }, 30, 20);
    row("no __restrict", plain);
    row("__restrict", restr, plain.p50);
    std::puts("    ^ often equal at -O2 because gcc versions the loop with a runtime");
    std::puts("      overlap check. Look at the asm (-S) to see which one vectorized;");
    std::puts("      __restrict removes the check and the fallback path. Lying about");
    std::puts("      it is UB.");
}

// ============================================ 8. algorithmic complexity wins
static void algorithm_beats_micro_optimization() {
    constexpr std::size_t kN = 20'000;
    std::vector<int> v(kN);
    std::mt19937 gen{11};
    std::iota(v.begin(), v.end(), 0);
    std::ranges::shuffle(v, gen);
    const int target = static_cast<int>(kN) - 1;         // worst case for linear scan

    std::vector<int> sorted = v;
    std::ranges::sort(sorted);

    std::puts("  8. algorithm vs. micro-optimization");
    const auto linear = bench([&] {
        do_not_optimize(std::find(v.begin(), v.end(), target) != v.end());
    }, 25, 50);
    const auto binary = bench([&] {
        do_not_optimize(std::binary_search(sorted.begin(), sorted.end(), target));
    }, 25, 5000);
    row("std::find (O(n))", linear);
    row("std::binary_search (O(log n))", binary, linear.p50);
    std::puts("    ^ no amount of SIMD or branch tuning closes an O(n) vs O(log n) gap.");
    std::puts("      Fix the algorithm first. (But: sorting cost O(n log n) once --");
    std::puts("      only worth it if you search many times.)");
}

// ============================================ 9. Amdahl's law, numerically
static void amdahl() {
    std::puts("  9. Amdahl's law: the ceiling on any local optimization");
    std::puts("    fraction optimized   2x faster   10x faster   infinitely faster");
    for (double p : {0.05, 0.20, 0.50, 0.90, 0.95}) {
        const auto speedup = [p](double s) { return 1.0 / ((1.0 - p) + p / s); };
        std::printf("        p = %4.0f%%          %5.2fx       %5.2fx        %5.2fx\n",
                    p * 100.0, speedup(2.0), speedup(10.0), speedup(1e9));
    }
    std::puts("    ^ making a 5% hotspot infinitely fast buys 1.05x. PROFILE FIRST.");
}

// ============================================ 10. Little's law
static void littles_law() {
    std::puts(" 10. Little's law: L = lambda * W (concurrency = throughput * latency)");
    const double stage_latency_ms = 5.0;
    for (double fps : {30.0, 100.0, 200.0}) {
        const double lambda = fps;                                 // frames / second
        const double w      = stage_latency_ms / 1000.0;           // seconds in stage
        std::printf("        %5.0f fps at %.1f ms per stage -> %.2f frames in flight\n",
                    fps, stage_latency_ms, lambda * w);
    }
    std::puts("    ^ so a 5 ms stage at 200 fps needs 1 concurrent unit; pipelining");
    std::puts("      raises THROUGHPUT and worsens end-to-end LATENCY, which is the");
    std::puts("      safety-relevant metric. You cannot tune them independently.");
}

int main() {
    std::puts("section 14: performance experiments (numbers are for THIS machine)");
    std::printf("  cache line assumed 64 B; sizeof(void*) = %zu\n\n", sizeof(void*));
    access_pattern();
    aos_vs_soa();
    struct_layout();
    branch_prediction();
    dependency_chains();
    allocation_cost();
    aliasing();
    algorithm_beats_micro_optimization();
    amdahl();
    littles_law();
    std::puts("\nsection 14: done -- now re-run under `perf stat` and explain each number.");
}
