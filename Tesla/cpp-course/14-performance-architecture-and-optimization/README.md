# 14 — Performance: Architecture, Code Optimization, Compilers, Profiling

Course chapters: **23 (Optimization I), 24 (Optimization II), 25 (Optimization III)**

The job posting: *"developing system tools to benchmark, characterize and optimize the
latency and throughput of the autonomy workloads on the Full-Self-Driving chip."*
This section **is** the job. Learn it properly.

---

## 1. The laws and the vocabulary

**Amdahl's law** — the speedup from optimizing a fraction *p* of the runtime by a
factor *s*:
```
speedup = 1 / ((1 - p) + p/s)
```
If 20% of the time is in a kernel you make infinitely fast, the best you can do is
1.25x. **Corollary: profile first, always.** The single most common mistake is
optimizing something that is 3% of runtime.

**Gustafson's law** — with more parallel hardware, you usually scale the *problem*,
not just the same work; so parallel speedup is not bounded the way Amdahl suggests for
strong scaling. Mention both and distinguish **strong scaling** (fixed problem, more
cores) from **weak scaling** (problem grows with cores).

**Little's law** — `L = λ × W`: concurrency = throughput × latency. This is the one to
quote in an autonomy interview: if a stage takes 5 ms and you need 200 frames/s, you
need 1 frame in flight per ms of latency → **1 concurrent unit per (λ·W)**. It also
says **you cannot improve throughput and latency independently**.

**Latency vs. throughput vs. bandwidth**: latency is time per operation, throughput is
operations per second, bandwidth is bytes per second. Pipelining raises throughput and
*worsens* latency. For autonomy the safety metric is **end-to-end latency at p99.9**,
not the mean and not throughput.

**Roofline / arithmetic intensity**: AI = FLOPs / bytes moved. Compare
`AI × peak_bandwidth` with `peak_FLOPS` to decide whether you are **memory-bound** or
**compute-bound**. Most autonomy kernels (convolutions aside) are memory-bound, which
is why data layout beats instruction-level tricks.

**Time-memory trade-off**: lookup tables, memoization, precomputation — but beware, a
table that blows the L1 cache is slower than recomputing. Measure.

## 2. Architecture facts you must have memorized

```
Latency numbers (typical modern server/automotive-class core, ~3 GHz)
  L1 hit                  ~1 ns     (4 cycles),   32-64 KB,  per core
  L2 hit                  ~4 ns     (12 cycles),  0.5-2 MB,  per core
  L3 hit                  ~15-40 ns (40-100 cyc), 8-64 MB,   shared
  DRAM                    ~80-100 ns
  Branch mispredict       ~15-20 cycles
  Atomic RMW (local L1)   ~5-20 cycles
  Atomic RMW (other core) ~100-200 cycles
  Mutex lock/unlock, uncontended  ~20 ns
  Context switch          ~1-5 µs
  Thread creation         ~10-30 µs
  malloc (fast path)      ~15-50 ns;  with a syscall, µs
  Page fault (minor)      ~1-3 µs
  SSD read                ~100 µs;    network RTT within a DC ~100 µs
```
**A cache line is 64 bytes** (128 on some ARM). If you remember one number, remember
that one and that a DRAM miss is ~100x an L1 hit.

Concepts to be able to define:
- **IPC** — instructions per cycle. Modern cores retire 4-6.
- **In-order vs. out-of-order execution**, the **reorder buffer**, and how OoO hides
  latency but not bandwidth.
- **Pipelining** — more stages = higher clock but more expensive mispredicts.
- **ILP** — instruction-level parallelism; limited by dependency chains. A long
  dependency chain (e.g. a serial FP sum) leaves most of the machine idle; this is why
  unrolling with multiple accumulators speeds up a reduction.
- **SIMD / DLP** — 128/256/512-bit registers (NEON on ARM, SSE/AVX/AVX-512 on x86);
  8 floats per AVX2 register, 4 per NEON.
- **SIMT** — the GPU/NPU model: many threads execute in lockstep; divergence costs.
- **TLP** — thread-level parallelism; hyperthreading shares execution units.
- **RISC vs. CISC** — ARM (the FSD chip) is RISC: fixed-width instructions, load/store
  architecture, more registers, weaker memory ordering (section 11).
- **Memory locality** — *temporal* (reuse soon) and *spatial* (use neighbours). The
  **hardware prefetcher** detects sequential and constant-stride access; it cannot
  follow a pointer chase.
- **TLB** — virtual→physical translation cache; a miss is a page walk. Huge pages
  (2 MB) cut TLB misses dramatically for large working sets.
- **NUMA** — on a multi-socket machine, memory attached to another socket is ~2x the
  latency; pin threads and allocate locally (`numactl`, `first-touch`).

## 3. Memory optimizations — the highest-yield category

**Layout first.** A 2x smaller struct is a 2x reduction in memory traffic for a linear
scan, and memory traffic is what most loops are limited by.

```cpp
// AoS: array of structures — good when you touch most fields of one element
struct Detection { float x, y, z, conf; std::uint32_t id; };
std::vector<Detection> dets;

// SoA: structure of arrays — good when a pass touches ONE field of every element
struct Detections {
    std::vector<float> x, y, z, conf;
    std::vector<std::uint32_t> id;
};
// A pass that only reads `conf` moves 4 bytes/element instead of 20, and vectorizes.
```
The other memory levers:
- **Order members big→small** to kill padding (`-Wpadded` finds it).
- **Shrink types**: `double`→`float` halves the traffic *and* doubles SIMD lanes;
  `std::uint16_t` indices instead of pointers.
- **`reserve()`** to avoid reallocation; **preallocate/pool** to avoid `malloc` in the
  hot path entirely.
- **Blocking/tiling** so the working set of the inner loops fits in L1/L2.
- **Align** hot data to a cache line; pad per-thread data to avoid **false sharing**.
- **Prefetch** (`__builtin_prefetch(ptr, 0, 3)`) only when the access pattern is
  predictable-by-you but not by the hardware (e.g. a linked structure you can look
  ahead in). Usually the answer is "fix the layout" instead.
- **Huge pages** for multi-GB working sets.
- Avoid **pointer chasing** — the prefetcher cannot help, so every node is ~100 ns.

## 4. Arithmetic, control flow, and functions

```cpp
// integer division and modulo are ~20-40 cycles; multiplication is 3-5
x / 8;  x % 8;                 // fine: compiler turns constant powers of 2 into shifts
x / runtime_divisor;           // expensive; hoist, or use libdivide, or restructure
std::midpoint(a, b);           // overflow-safe (a+b)/2

// float vs double: half the bytes, twice the SIMD lanes, and on some hardware
// meaningfully faster transcendentals. Use float unless you can justify double.
std::fma(a, b, c);             // one rounding instead of two, and one instruction

// branches
if (x > 0) [[likely]] { }      // C++20 hint; only use it when you KNOW
                               // (a mispredict costs ~15-20 cycles)
// branchless alternatives when the branch is unpredictable:
int m = (a > b) ? a : b;       // the compiler usually emits cmov here anyway
count += (x > threshold);      // no branch at all
std::clamp(v, lo, hi);
// Sorting the input to make branches predictable is a real, measurable technique.

// loops
for (std::size_t i = 0, n = v.size(); i < n; ++i)   // hoist the bound
// multiple accumulators break the dependency chain and let the FPU pipeline:
double s0=0, s1=0, s2=0, s3=0;
for (i = 0; i + 3 < n; i += 4) { s0+=a[i]; s1+=a[i+1]; s2+=a[i+2]; s3+=a[i+3]; }

// functions
inline / [[gnu::always_inline]] / [[gnu::noinline]]
// pointer ALIASING is the #1 reason a loop does not vectorize:
void add(float* __restrict out, const float* __restrict a, const float* b, int n);
[[gnu::pure]] / [[gnu::const]]  // lets the compiler CSE calls
```
**Aliasing** is worth a paragraph in the interview: given `void f(float* a, float* b)`,
the compiler must assume `a` and `b` may overlap, so it cannot keep values in registers
across a store, cannot reorder, and cannot vectorize. `__restrict` is the promise that
they do not — and it is UB if you lie. `std::span` does not help; `__restrict` or
value-semantics do.

**Recursion** costs a call frame per level and blocks many optimizations; convert to
iteration when the depth is data-dependent (also removes stack-overflow risk).

**Virtual calls** — see section 06: the cost is lost inlining, not the jump.

## 5. Compiler flags and what they actually do

```bash
-O0   no optimization, fast build, debuggable        # never benchmark this
-O1   basic; already does a lot
-O2   the default for production: inlining, vectorization (gcc 12+), CSE, etc.
-O3   more aggressive inlining and loop transforms; sometimes SLOWER (i-cache)
-Os / -Oz   optimize for size (matters on an embedded target)
-Og   optimized but debuggable

-march=native / -march=armv8.2-a+simd   # ENABLE THE ISA. Often the single biggest win
-mtune=...                  # schedule for a microarchitecture without requiring it
-flto                       # link-time optimization: cross-TU inlining + devirt
-fprofile-generate / -fprofile-use      # PGO: 5-20% typical, more on branchy code
-fno-exceptions -fno-rtti   # size and some codegen, on embedded
-ffast-math                 # DANGEROUS: see section 01. Prefer the pieces you need:
   -fno-math-errno -ffp-contract=fast -fassociative-math
-fno-semantic-interposition -fvisibility=hidden   # better inlining in shared libs
-g -fno-omit-frame-pointer  # keep these in production: profiling needs them
```
**Compiler transformations to name**: inlining, constant folding/propagation, common
subexpression elimination, dead-code elimination, strength reduction, loop-invariant
code motion (hoisting), loop unrolling, unswitching, fusion, fission/distribution,
interchange, tiling/blocking, vectorization, tail-call elimination, devirtualization.

**PGO/AutoFDO**: build instrumented, run a representative workload, rebuild with the
profile. It improves branch layout, inlining decisions, and code placement (i-cache).
5-20% for free, and it is exactly the kind of thing a platform team owns.

**BOLT / post-link optimizers** re-lay-out the binary from a `perf` profile for i-cache
locality — another 5-15% on large binaries.

## 6. Measuring — the part people get wrong

Rules for a benchmark that means something:
1. **Never benchmark `-O0`.** Never benchmark a debug build.
2. **Stop the compiler deleting your work**: use the result, or
   `benchmark::DoNotOptimize(x)` / an empty `asm volatile("" :: "r"(x) : "memory")`.
3. **Warm up** (i-cache, d-cache, branch predictor, frequency ramp), then measure many
   iterations.
4. **Report percentiles, not the mean.** p50/p99/p99.9 and max. For a deadline-driven
   system the tail *is* the metric.
5. **Pin the CPU** (`taskset`), disable frequency scaling and turbo, use
   `cpupower`/`performance` governor, isolate the core.
6. **Run A and B interleaved**, many times, and check the variance. Machine state
   drifts.
7. **Measure the right thing**: `steady_clock` for wall time, `perf` counters for
   *why*, and both the benchmark and the real workload.

```cpp
// A correct minimal timing harness
template <typename F>
double time_ns(F&& f, int iters) {
    for (int i = 0; i < iters / 10; ++i) f();            // warm up
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < iters; ++i) f();
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / iters;
}
template <typename T> void do_not_optimize(T const& v) {
    asm volatile("" : : "r,m"(v) : "memory");
}
```

**`perf`, the commands to know:**
```bash
perf stat ./app                          # IPC, branches, cache misses -- START HERE
perf stat -e cycles,instructions,cache-references,cache-misses,\
branch-misses,LLC-load-misses,page-faults ./app
perf record -g --call-graph dwarf ./app  # sampling profile
perf report --sort=dso,symbol
perf annotate symbol                     # per-instruction attribution
perf top                                 # live
perf c2c record/report                   # FALSE SHARING detector
perf sched latency                       # scheduler wakeup latency
perf trace / ftrace / bpftrace           # syscalls and kernel events
```
What to look at first: **IPC** (< 1 means stalls — memory or mispredicts),
**cache-miss rate**, **branch-miss rate**, then the hot symbols. A memory-bound loop
shows low IPC and high LLC misses; a branchy one shows high branch-misses.

Other tools by name: **valgrind --tool=cachegrind / callgrind** (exact, slow),
**heaptrack/massif** (allocations), **google-benchmark** and **quick-bench.com**
(microbenchmarks), **Intel VTune / ARM Streamline** (vendor profilers),
**Compiler Explorer** (read the asm), **`llvm-mca`** (static throughput analysis of a
loop body).

## 7. Parallelism

Concurrency (structure: several tasks in progress) vs. parallelism (execution: several
tasks at once). Options in rough order of effort: `std::execution::par` algorithms,
OpenMP (`#pragma omp parallel for`), a thread pool, TBB, and then SIMD intrinsics /
`std::simd` (C++26) / compiler auto-vectorization, and finally the accelerator (NPU,
GPU, DSP).

Scaling is limited by Amdahl, by **memory bandwidth** (adding cores does not add
bandwidth), by synchronization, and by **false sharing**. Always measure scaling
efficiency, not just "it got faster".

---

## The optimization workflow (say this, in this order)

1. **Set a target.** "p99 end-to-end under 100 ms" — not "make it faster".
2. **Measure** with a representative workload. `perf stat` first, then `perf record`.
3. **Find the actual bottleneck** and check Amdahl: is it worth it?
4. **Fix the algorithm first** (O(n²)→O(n log n) beats any micro-optimization).
5. **Then data layout and memory** (AoS→SoA, shrink, pool, tile).
6. **Then the compiler** (`-march`, LTO, PGO) — often free.
7. **Then micro-optimizations** (branchless, `__restrict`, intrinsics).
8. **Re-measure, and guard it with a benchmark in CI** or it regresses.

---

## Traps checklist

1. Profile before optimizing; Amdahl bounds your best case.
2. Never benchmark `-O0`, and always defeat dead-code elimination.
3. Report p99/p99.9, not the mean, for anything with a deadline.
4. Cache line = 64 B; DRAM ≈ 100x L1. Layout beats instruction tricks.
5. Pointer chasing defeats the prefetcher.
6. Aliasing is the #1 reason a loop will not vectorize.
7. `-O3` is not always faster than `-O2` (i-cache pressure).
8. `-march=native` is often the single biggest one-flag win — and it breaks portability.
9. `-ffast-math` changes results and deletes NaN checks (section 01).
10. Adding cores does not add memory bandwidth.
11. Pipelining improves throughput and *worsens* latency.
12. Keep `-g -fno-omit-frame-pointer` in production so you can profile it.
