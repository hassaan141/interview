# 14 — Mock interview questions

This section is the job description. C1-C4 are the shape of question you should expect.

## A. Rapid fire — numbers

1. Cost of an L1 hit, L2 hit, L3 hit, DRAM access. Ratio of DRAM to L1?
2. Size of a cache line?
3. Cost of a branch mispredict? A context switch? Thread creation?
4. Cost of an atomic RMW when the line is in local L1? When another core owns it?
5. Cost of an uncontended mutex lock? A contended one?
6. How many floats fit in an AVX2 register? A NEON register?
7. How many instructions per cycle does a modern core retire?

## A2. Rapid fire — concepts

8. State Amdahl's law. If a hotspot is 20% of runtime, what is the best possible speedup?
9. State Little's law and give an autonomy example.
10. Difference between latency, throughput, and bandwidth. Which one does pipelining hurt?
11. What is arithmetic intensity, and what do you do with it?
12. Strong vs. weak scaling.
13. What is ILP? Why does a single FP accumulator run at 1/4 speed?
14. Which access patterns can the hardware prefetcher handle?
15. What is a TLB miss, and when do huge pages help?
16. What is false sharing and how do you detect it?
17. AoS vs. SoA — when is each right?
18. Why does `-O3` sometimes lose to `-O2`?
19. What does `-march=native` do, and what is the catch?
20. What is LTO? What is PGO? Typical wins?
21. Name three things that stop a loop from vectorizing.
22. What does `__restrict` promise, and what happens if you lie?
23. Why must you never benchmark at `-O0`? Name two other benchmarking mistakes.
24. Why report p99.9 instead of the mean?
25. What is the first `perf` command you run, and what three numbers do you read?
26. What does `perf c2c` find? `perf sched latency`?
27. What does IPC < 1 tell you? How do you narrow it down?
28. Adding cores does not add what?

## B. Diagnose the performance problem

**B1.** `perf stat`: IPC 0.35, cache-misses 42% of references, branch-misses 1%.

**B2.** `perf stat`: IPC 0.8, branch-misses 18% of branches, cache-misses 2%.

**B3.** IPC 2.8, low cache misses, low branch misses — but it still misses the deadline.

**B4.** A loop over 10M elements is 4x slower than a colleague's equivalent loop. The
asm shows `addss` (scalar) where theirs shows `vaddps` (packed).

**B5.** The processing thread is 30% faster after you added a second thread doing
unrelated statistics counting.

**B6.** p50 latency is 4 ms; p99.9 is 85 ms. Deadline is 20 ms.

**B7.** Scaling from 1 to 8 threads gives 2.1x.

**B8.** Enabling `-flto` made the binary 20% faster but the build 3x slower and one
test now fails.

**B9.** A function that does `std::vector<float> tmp(n)` internally shows up as 25% of
frame time, all of it in `operator new` and `memset`.

**B10.** Replacing `std::map` with `std::unordered_map` made the code *slower*.

## C. Whiteboard

**C1.** You are asked to build the tooling to "benchmark, characterize and optimize
the latency and throughput of the autonomy workloads on the FSD chip". Design it.

**C2.** A perception stage takes 14 ms and must fit in 8 ms. Walk through your
optimization process, in order, with what you would measure at each step.

**C3.** Optimize this function. State every assumption and the expected win of each
change.
```cpp
std::vector<Detection> filter_and_score(const std::vector<Detection>& in,
                                        float min_conf) {
    std::vector<Detection> out;
    for (int i = 0; i < in.size(); i++) {
        if (in[i].confidence > min_conf) {
            Detection d = in[i];
            d.score = std::sqrt(d.x*d.x + d.y*d.y + d.z*d.z) * d.confidence;
            out.push_back(d);
        }
    }
    return out;
}
```

**C4.** Design a low-overhead tracing API that every autonomy stage can use in a 100 Hz
loop to produce per-stage latency histograms, with less than 1% overhead.

---
---

# Answers

**A1.** L1 ~1 ns (4 cycles), L2 ~4 ns (12 cycles), L3 ~15-40 ns (40-100 cycles), DRAM
~80-100 ns. **DRAM is roughly 100x an L1 hit.** That ratio is the single most important
number in performance work.

**A2.** **64 bytes** on x86-64 and most ARM (128 on Apple M-series and some server ARM;
`std::hardware_destructive_interference_size` tells you).

**A3.** Branch mispredict ~15-20 cycles (the pipeline depth). Context switch ~1-5 µs.
Thread creation ~10-30 µs. Minor page fault ~1-3 µs.

**A4.** Line in local L1, uncontended: ~5-20 cycles. Line owned by another core: the
coherence protocol must transfer exclusive ownership, ~100-200 cycles — and with several
cores hammering it, throughput collapses (that is false sharing).

**A5.** Uncontended: ~20 ns, all in userspace (a futex only enters the kernel on
contention). Contended: a syscall plus a context switch, ~1-10 µs. A ~100x cliff, which
is why critical sections must be short.

**A6.** AVX2 is 256 bits → **8 floats** (or 4 doubles). AVX-512 → 16 floats. NEON is
128 bits → **4 floats** (2 doubles). Hence `double`→`float` doubles your vector width
as well as halving memory traffic.

**A7.** 4-6 instructions per cycle on a modern wide out-of-order core — so an IPC of
1.0 means you are using ~20% of the machine.

**A8.** `speedup = 1 / ((1-p) + p/s)`. For p = 0.20, even with s = ∞ the speedup is
`1/0.8 = 1.25x`. The corollary is the actual lesson: **profile first**, because the
ceiling on optimizing a small fraction is small no matter how clever you are.

**A9.** `L = λ × W`: the average number of items in a system equals arrival rate times
time in system. Example: a perception stage with 5 ms latency at 200 frames/s needs
`200 × 0.005 = 1` frame in flight; at 8 ms and 200 fps you need 1.6, so you need at
least 2 concurrent workers or the queue grows without bound. It also tells you that
throughput and latency are coupled through concurrency — you cannot tune one without
affecting the others.

**A10.** **Latency** = time for one operation (seconds). **Throughput** = operations per
second. **Bandwidth** = bytes per second. **Pipelining improves throughput and makes
end-to-end latency worse** (you add handoff, queueing, and cache effects). For autonomy
the safety-relevant number is end-to-end sensor→actuation **latency at the tail**, not
throughput.

**A11.** AI = FLOPs performed / bytes moved from memory. Compare
`AI × achievable_bandwidth` against `peak_FLOPS`: if the former is smaller you are
**memory-bound** (optimize data movement — layout, tiling, precision), if larger you are
**compute-bound** (optimize instructions — SIMD, FMA, better algorithm). That is the
roofline model, and it tells you which of the two categories of optimization is even
worth attempting.

**A12.** **Strong scaling**: fixed problem size, add cores, measure speedup — bounded by
Amdahl. **Weak scaling**: grow the problem with the cores, measure whether time stays
constant — Gustafson's framing, and usually the honest one for data-parallel workloads.

**A13.** Instruction-level parallelism: independent instructions the out-of-order core
can execute simultaneously. A single FP accumulator creates a **serial dependency
chain** — each add must wait for the previous one's result (~4 cycle latency) even
though the unit can start 1-2 adds *per cycle*. Four independent accumulators fill the
pipeline, which is the ~4x measured in `examples.cpp`.

**A14.** Sequential (stride 1) forward or backward, and constant strides within a page,
and a small number of concurrent streams. It **cannot** follow a pointer chase (each
address depends on the previous load), nor an indirect/gather pattern, nor a random
permutation — hence the ~32x measured in `examples.cpp`.

**A15.** The TLB caches virtual→physical page translations; a miss requires a page-table
walk (several dependent memory accesses). With 4 KB pages, a 1 GB working set needs
262k translations and will thrash a ~1500-entry TLB. **2 MB huge pages** cover 512x more
memory per entry, so they help any workload with a large, randomly accessed working
set — commonly 5-20%. They cost memory (internal fragmentation) and can add latency
spikes if the kernel has to compact memory to find them (`transparent_hugepage=madvise`
rather than `always`).

**A16.** Two threads writing **different** variables that share a cache line: each write
invalidates the other core's copy, so the line ping-pongs. Detect with **`perf c2c`**
(purpose-built: it reports HITM events and the exact cache-line offsets), or
`perf stat -e cache-misses` plus an A/B test with padding. Fix with
`alignas(std::hardware_destructive_interference_size)`, per-thread accumulators merged
at the end, or splitting the array by thread.

**A17.** **SoA** when a pass touches one or a few fields of every element (you move only
what you use, and it vectorizes) — the common case for a filter/score/reduce pipeline.
**AoS** when a pass touches most fields of one element (one cache line brings the whole
object), and when elements are individually created/destroyed/passed around. In
`examples.cpp` SoA is ~3x faster for a single-field pass. Real answer: pick per access
pattern, and consider AoSoA (tiles of 8 or 16) to get both.

**A18.** `-O3` adds aggressive inlining, loop unrolling, and vectorization, which can
**bloat code** and thrash the instruction cache, and can turn a small predictable loop
into a large one with more prologue/epilogue overhead. On a branchy, i-cache-bound
workload `-O2` (or `-O2 -march=native`) often wins. Measure per binary; do not assume.

**A19.** It enables every ISA extension the *build* machine supports (AVX2, AVX-512,
FMA, BMI) and tunes scheduling for it. Often the single largest one-flag win because
without it the compiler targets a baseline from 2003. **The catch**: the binary crashes
with SIGILL on any machine lacking those extensions — so for shipped software use an
explicit baseline (`-march=x86-64-v3`, `-march=armv8.2-a+simd`) plus runtime dispatch
(`__builtin_cpu_supports`, `target_clones`, or ifunc) for the hot kernels.

**A20.** **LTO** defers optimization to link time so the compiler sees the whole program:
cross-TU inlining, devirtualization, better dead-code elimination, and `-Wodr` as a
bonus. Typical 5-15%, more if your hot path crosses TU boundaries. **PGO** builds
instrumented, runs a representative workload, then rebuilds using the profile to improve
branch layout, inlining, and code placement. Typical 5-20%, more on branchy code (10-30%
on compilers/interpreters). Both are "free" performance a platform team should own; the
cost is build complexity and needing a representative workload.

**A21.** (1) **Pointer aliasing** — the compiler must assume the output may overlap an
input (fix: `__restrict`, or value semantics); (2) a **loop-carried dependency**
(including a serial FP reduction, which needs reassociation permission); (3) a **function
call** in the body that did not inline; (4) non-unit or unknown **stride**; (5) control
flow in the body that cannot be if-converted; (6) an unknown trip count or possible
overflow of the induction variable. Diagnose with `-fopt-info-vec-missed` /
`-Rpass-missed=loop-vectorize`.

**A22.** That for the lifetime of the pointer, the object it points to is accessed
**only** through that pointer (no other pointer aliases it). This lets the compiler keep
values in registers across stores, reorder, and vectorize. If you lie it is **undefined
behavior** — the compiler will happily produce a loop that computes the wrong answer for
overlapping ranges, and there is no diagnostic.

**A23.** At `-O0` the compiler emits naive code that mirrors the source, so you measure
the code generator's laziness rather than your algorithm — relative results can even
invert. Other mistakes: (1) letting **dead-code elimination delete the work** (use the
result, or `DoNotOptimize`); (2) **no warmup**, so you measure cold i-cache, cold
branch predictor, and CPU frequency ramp; (3) reporting the **mean of one run**;
(4) not pinning the CPU / leaving turbo and frequency scaling on; (5) running A then B
rather than interleaved, so machine drift becomes your "result"; (6) an unrepresentative
input (all-zero data compresses and predicts differently).

**A24.** Because a deadline-driven system fails on its **tail**, not its average. A 100 Hz
loop runs 8.6M times a day; a p99.9 of 85 ms means ~8600 deadline misses per day, which
a 4 ms mean completely hides. Percentiles also reveal the *shape* — a bimodal
distribution points at a specific rare event (a rehash, a page fault, a lock) which an
average cannot.

**A25.** `perf stat ./app`. Read: (1) **insn per cycle** — under ~1.0 means the core is
stalling; (2) **cache-misses as a fraction of cache-references** — high means memory
bound; (3) **branch-misses as a fraction of branches** — high means unpredictable
control flow. Then `stalled-cycles-frontend` vs `-backend` to split "cannot fetch
instructions" from "waiting on data".

**A26.** `perf c2c` ("cache to cache") finds **false sharing and true sharing**: it
samples HITM (hit-modified) events and reports the specific cache lines and the byte
offsets within them that are being contended, plus which threads. `perf sched latency`
reports per-task **scheduler wakeup delay** — how long a runnable thread waited for a
CPU, which is how you prove that a missed deadline was jitter and not compute.

**A27.** The core is stalling most cycles. Narrow it down: high
`stalled-cycles-backend` + high LLC misses → **memory bound** (fix layout);
high `stalled-cycles-frontend` → i-cache/ITLB misses or mispredicts (fix code size,
PGO/BOLT); high `branch-misses` → control flow; long dependency chains with low miss
rates → **latency bound on the dependency chain** (fix with ILP/unrolling); and if none
of those, look for serializing instructions (division, `sqrt`, atomics, fences).

**A28.** **Memory bandwidth.** All cores share the same path to DRAM, so a
bandwidth-bound kernel stops scaling well before you run out of cores — which is why
reducing bytes moved (SoA, smaller types, tiling) often beats parallelizing. Also: cores
do not add I/O bandwidth, cache capacity per core, or fix Amdahl's serial fraction.

---

**B1.** Classic **memory bound**. 42% miss rate with low branch misses says the loop is
waiting on DRAM. Look at the access pattern and the data layout: is it a pointer chase,
a random-index gather, or a struct where you read one field of 20 bytes? Fixes in order:
SoA, shrink the types, tile so the working set fits L2, remove pointer indirection (index
instead of pointer), and only then consider prefetching. Confirm with
`perf c2c`/`perf mem` and with a version of the loop over a small array that fits in L1
(if it gets 30x faster, it is definitely memory).

**B2.** **Branch-bound.** 18% mispredict rate is very high — the data-dependent
condition is essentially random. Fixes: make it branchless (`cmov`, arithmetic on the
predicate, `std::clamp`), sort or partition the input so the branch becomes predictable
(the 6x in `examples.cpp`), use a lookup table, use SIMD with masks, or hoist the
condition out of the loop (loop unswitching). Do *not* reach for `[[likely]]` — that
helps a *biased* branch, not an unpredictable one.

**B3.** The core is running efficiently, so the problem is not the microarchitecture:
you are either doing **too much work** (wrong algorithm — check complexity and redundant
computation) or you are **not running** when you should be. Check the second first:
`perf sched latency` for wakeup delay, scheduling policy and affinity, whether another
process shares the core, and whether there is blocking (I/O, a lock, an allocation that
hit the kernel) — `perf trace`, and measure *wake-up time* separately from *duration*.
Also check that you are measuring the deadline end-to-end and not just this stage.

**B4.** Their loop **vectorized** and yours did not (`addss` = one float at a time;
`vaddps` = 8). Most likely cause: **pointer aliasing** — add `__restrict`, or pass by
value/`std::span` with non-overlapping guarantees. Other candidates: a serial FP
reduction (needs `-ffp-contract`/reassociation or multiple accumulators), a
non-inlined call in the body, a non-unit stride, or a missing `-march=`. Diagnose
precisely with `-fopt-info-vec-missed` (gcc) or `-Rpass-missed=loop-vectorize` (clang) —
it names the loop and the reason.

**B5.** **30% faster** after *adding* work means the original was suffering from
something the new thread accidentally relieved — but far more likely you have a
measurement artifact. Check: did the frequency governor ramp up because the second
thread kept the core busy (turbo/idle-state exit latency)? Did the counter thread warm
a shared cache or the TLB? Did the OS migrate the processing thread onto a less
contended core? Reproduce with the CPU pinned, the governor set to `performance`, and
turbo disabled — the effect usually disappears. If it survives, suspect a
`C-state`/`idle` wake-up latency issue and consider `cpu_dma_latency`/`idle=poll` for
the RT core. The lesson to state: an unexplained speedup is a measurement bug until
proven otherwise.

**B6.** A 20x tail means a **rare discrete event**, not general slowness. Candidates, in
order of likelihood: an allocation that hit the kernel (`brk`/`mmap`) or a page fault; a
container growth/rehash; a lock contention window or priority inversion; a logging or
I/O call; a destructor storm; scheduler preemption or migration; a cache/TLB cold
period. Method: instrument with an always-on ring-buffer tracer that is **dumped only
when the deadline is missed** (you cannot afford to log every iteration, and you cannot
reproduce a 1-in-1000 event interactively). Split wake-up time from duration to separate
scheduling jitter from compute. Then fix the cause — preallocate, `reserve`, move
logging off the hot path, pin and isolate the core — and gate p99.9 in CI.

**B7.** 2.1x from 8 threads is 26% efficiency. Work through the candidates: (1) **Amdahl**
— measure the serial fraction; 2.1x implies ~40% serial. (2) **Memory bandwidth
saturation** — check with `perf stat` whether per-thread throughput drops as threads are
added; if the kernel is bandwidth-bound, no thread count helps and you must reduce bytes
moved. (3) **False sharing** — `perf c2c`. (4) **Lock contention** — `perf stat -e
'syscalls:sys_enter_futex'`, or a contention profiler. (5) **Load imbalance** — measure
per-thread work; a static split over non-uniform work leaves cores idle (fix: dynamic
chunking / work stealing). (6) **Hyperthreading** — 8 "threads" may be 4 physical cores
sharing execution units. Report the scaling *curve*, not one point.

**B8.** Two separate findings. The 20% is real and expected (cross-TU inlining and
devirtualization). The **3x build time** is the known cost — mitigate with **ThinLTO**
(`-flto=thin`), which is near-parallel and gets most of the win, plus `ccache`. The
**failing test** is the important one: LTO commonly *exposes* latent UB, because
whole-program visibility lets the optimizer act on assumptions it previously could not
(strict aliasing across TUs, an ODR violation now visible to `-Wodr`, UB the optimizer
now propagates). Do not disable LTO — run the failing test under
`-fsanitize=address,undefined`, read the `-Wodr` warnings from the LTO link, and fix the
real bug. That answer is what distinguishes someone who has actually rolled out LTO.

**B9.** The function allocates a temporary on every call. `operator new` plus the
`memset` from value-initialization is pure overhead. Fixes, in order: (1) hoist the
buffer out and pass it in (`std::span<float> scratch`), or make it a member, or use a
thread-local arena — this removes the allocation *and* the zeroing; (2) if the size is
bounded, use a fixed-capacity inline buffer (`StaticVector<float, N>`) so it lives on the
stack; (3) if you must allocate, `reserve` once and reuse, and avoid the zero-init by
constructing with `resize` only when needed or using uninitialized storage. Expected win
here: most of the 25%. This is the single most common finding in a real-time C++ profile.

**B10.** Several real reasons, and naming them shows depth: (1) **n is small** — for
under ~30-50 elements a red-black tree (or better, a sorted vector) beats hashing
because the hash computation and cache miss dominate; (2) `std::unordered_map` is
**node-based**, so each lookup is a bucket load plus a pointer chase to a separately
allocated node — two cache misses, versus a tree walk that may be entirely in L1 for a
small map; (3) the **hash is poor** (e.g. `std::hash<int>` is the identity on libstdc++,
so sequential keys with a power-of-two bucket count can cluster), degrading to O(n);
(4) the workload needed **ordered iteration or range queries**, which now costs a sort;
(5) memory grew (bucket array + nodes), evicting other hot data. Right move: measure,
and try a **sorted `std::vector` + `lower_bound`** or `absl::flat_hash_map` — contiguous
storage usually beats both standard containers.

---

**C1.** Design for "benchmark, characterize, and optimize latency and throughput on the
FSD chip":

**(a) Instrumentation layer — the foundation.**
A header-only tracing API every stage uses, with < 1% overhead (see C4): a
`TRACE_SPAN("undistort")` RAII object writing `{stage_id, thread_id, start_tsc,
end_tsc}` into a **per-thread lock-free ring buffer**, drained by a low-priority thread.
Timestamps from a monotonic counter (`rdtsc`/`cntvct_el0`) with a single calibration, not
`clock_gettime` per event. Compile-time selectable so it can be fully removed.

**(b) Metrics, not averages.**
Per stage and end to end: a **latency histogram** (HdrHistogram-style, log-bucketed, so
p50/p99/p99.9/max are exact and cheap), throughput, queue depths (Little's law tells you
what they should be), drop counts, and CPU/NPU utilization. End-to-end
**sensor-timestamp → actuation-timestamp**, which is the only number that matters for
safety.

**(c) A microbenchmark suite** for the foundational primitives (the ring buffer, the
allocator, the parser, the math kernels) with google-benchmark, run on the **target
hardware**, pinned, with the governor fixed, results stored as a time series.

**(d) A macrobenchmark / replay harness**: run the whole stack over a fixed corpus of
logged drives on the target board, deterministically (section 13 C3), and emit the
metrics above per run.

**(e) Characterization, not just measurement.** For each workload, report where it sits
on the **roofline**: arithmetic intensity, achieved bandwidth vs. peak, achieved FLOPS vs.
peak, IPC, cache-miss rates, NPU occupancy. That is what tells the application teams
*which kind* of optimization will pay, and it is what "characterize" means in the
posting.

**(f) Regression gating.** Store every result keyed by commit; fail CI on a p99
regression beyond a budget; auto-bisect. Track binary size and build time the same way.
Crucially: **the budget is absolute** ("p99 end-to-end < 100 ms"), not relative, because
relative gates let you drift.

**(g) A profiling workflow the application teams can use themselves**: a one-command
wrapper that runs a scenario with `perf record` on the board, symbolizes, and produces a
flame graph plus a `perf stat` summary — because a platform team's leverage comes from
making 50 other engineers able to self-serve.

Say the ordering explicitly: instrumentation → metrics → baselines → gates → self-serve
tooling. And name the constraint that shapes all of it: **it must run on the target, at
production rates, with negligible overhead**, or the numbers are fiction.

**C2.** 14 ms → 8 ms, a 1.75x needed.
1. **Define the target precisely**: 8 ms at which percentile? p99 on the target SoC with
   production data. Get a reproducible benchmark first — pinned core, fixed governor,
   representative input, percentile output. Without this, everything after is guesswork.
2. **`perf stat`**: IPC, cache misses, branch misses, page faults. This tells you which
   *category* of problem you have in one command (see B1-B3).
3. **`perf record -g`**: find where the 14 ms is. Apply **Amdahl** before touching
   anything: if the top function is 15%, making it free buys 1.18x and you do not get to
   8 ms that way — you need the top 60-70% of the profile.
4. **Algorithm and redundant work first.** Is anything O(n²)? Recomputed per frame when
   it could be cached? Processing the full image when a region of interest suffices?
   Running at a higher precision or resolution than the downstream consumer needs? The
   biggest wins in real perception code are usually "do less", not "do it faster".
5. **Data layout and allocation.** AoS→SoA for the single-field passes, shrink
   `double`→`float`, remove hot-path allocation (preallocate/pool), tile so the working
   set fits L2, fix false sharing. Expect the largest microarchitectural win here if step
   2 said memory-bound.
6. **Compiler, which is nearly free**: `-march=` for the actual target, `-O2` vs `-O3`
   measured, **LTO**, **PGO** on a representative scenario. 10-30% combined is typical.
7. **Vectorize the kernels**: check `-fopt-info-vec-missed`, fix aliasing with
   `__restrict`, restructure for unit stride, and only hand-write intrinsics (or use the
   NPU/DSP) if the compiler cannot be persuaded.
8. **Parallelize** — within the stage (data-parallel over image tiles) rather than
   across frames, because across-frame pipelining improves throughput but worsens the
   end-to-end latency you are trying to fix. Check scaling efficiency and memory
   bandwidth.
9. **Re-measure after each change**, keep the ones that pay, and **lock it in** with a
   CI benchmark and an absolute p99 budget.
10. If after all that it is 9 ms: go back to the product requirement — can the stage run
    at a lower resolution, or can the deadline be met by reordering the pipeline so this
    stage overlaps another? Stating that engineering-negotiation step is part of a good
    answer.

**C3.**
```cpp
// Assumptions I would state out loud:
//  - `in` is large (thousands+) and this runs every frame in the hot path
//  - selectivity is unknown; assume a meaningful fraction passes
//  - Detection is trivially copyable and `score` is the only field written
//  - callers can own the output buffer (the real-time form), and a float `score`
//    is enough precision
//  - x,y,z,confidence are float; std::sqrt on double was an accidental promotion

// 1) Caller-owned output: no allocation in the hot path at all.  [biggest win]
// 2) span parameters: no coupling to std::vector, no (ptr,len) mismatch.
// 3) std::size_t index (or a range-for): `int i < in.size()` is a signed/unsigned
//    comparison warning and breaks past 2^31 elements.
// 4) compute the score with floats and std::hypot-free arithmetic; keep it in
//    registers; no per-element copy of the whole struct until it is kept.
[[nodiscard]] std::span<Detection>
filter_and_score(std::span<const Detection> in, std::span<Detection> out,
                 float min_conf) noexcept {
    std::size_t n = 0;
    for (const Detection& d : in) {                 // 5) range-for: no bound reload
        if (d.confidence <= min_conf) continue;     // 6) early-continue, flat body
        Detection kept = d;                          // copy only the survivors
        const float r2 = d.x * d.x + d.y * d.y + d.z * d.z;
        kept.score = std::sqrt(r2) * d.confidence;   // 7) float sqrt, not double
        out[n++] = kept;
    }
    return out.first(n);
}
```
Expected wins, in order:
- **Removing the allocation and the repeated reallocation** (`out` was default
  constructed and grown): this is usually the dominant cost — one `malloc` plus
  log₂(n) reallocations, each copying everything so far. Caller-owned `out`, or at
  minimum `out.reserve(in.size())`. **Largest single win; often 2-5x on this shape of
  function.**
- **`std::sqrt(double)` → `float`**: the original promotes `float` arithmetic to
  `double` inside `sqrt` and back (`-Wdouble-promotion` catches it), which halves your
  SIMD width and costs a slower instruction. Use `std::sqrt` on a `float` (or
  `sqrtf`). Also note `sqrt` is ~15-20 cycles and cannot be removed — but if the caller
  only *ranks* by score, you can compare `r2 * conf²` and skip the sqrt entirely. **Ask
  whether the sqrt is needed at all** — that is the best answer available.
- **`int i` → `std::size_t` / range-for**: correctness (signed/unsigned comparison,
  >2 GB inputs) and it lets the compiler keep the bound in a register.
- **Layout**: if this is one of several passes that each read one or two fields, convert
  `Detection` to **SoA** — `conf` becomes a contiguous `float` array, the filter
  vectorizes into a mask+compress, and you move 4 bytes per element instead of
  `sizeof(Detection)`. ~3x for the scan (measured in `examples.cpp`).
- **Vectorization**: with SoA and `__restrict`, the whole thing becomes a masked
  compute + compaction (`_mm256_maskstore`/AVX-512 `compress`, or NEON equivalents).
  The compiler will do it if you remove the aliasing and the conditional `push_back`.
- **`noexcept` + `[[nodiscard]]`**: contract improvements, and `noexcept` removes the
  unwind path.
Then say the discipline: **measure before and after each change**, and put a benchmark
in CI. And flag the one design question — whether the caller wants owning or
non-owning output — because it changes the API and only the caller can answer it.

**C4.**
```cpp
// Target: < 1% of a 10 ms budget = < 100 us per frame for ALL tracing, across
// (say) 50 spans per frame -> a budget of ~2 us per span, which is enormous.
// So the real constraints are: no allocation, no lock, no syscall, no cache-line
// sharing between threads, and no clock_gettime in the inner loop.

using Tsc = std::uint64_t;
inline Tsc now_tsc() noexcept {
#if defined(__x86_64__)
    std::uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));      // ~20 cycles, no syscall
    return (static_cast<Tsc>(hi) << 32) | lo;
#elif defined(__aarch64__)
    Tsc v; asm volatile("mrs %0, cntvct_el0" : "=r"(v)); return v;
#endif
}

struct Event { std::uint32_t stage_id; std::uint32_t depth; Tsc begin, end; };

// One ring per thread: no sharing, no atomics on the hot path, no false sharing.
class alignas(std::hardware_destructive_interference_size) ThreadTrace {
    static constexpr std::size_t kCap = 1 << 14;         // power of two
    std::array<Event, kCap> ring_{};
    std::size_t write_{0};                                // thread-local: plain int
public:
    void record(const Event& e) noexcept {
        ring_[write_ & (kCap - 1)] = e;                   // mask, never a modulo
        ++write_;                                          // overwrite oldest: bounded
    }
    std::span<const Event> snapshot() const noexcept { /* for the drain thread */ }
};
inline thread_local ThreadTrace t_trace;

// The user-facing API: an RAII span, fully compiled out when disabled.
class TraceSpan {
    Tsc begin_;
    std::uint32_t id_;
public:
    explicit TraceSpan(std::uint32_t stage_id) noexcept
        : begin_{now_tsc()}, id_{stage_id} {}
    ~TraceSpan() noexcept { t_trace.record({id_, 0, begin_, now_tsc()}); }
    TraceSpan(const TraceSpan&) = delete;
};
#if TRACING_ENABLED
#  define TRACE_SPAN(id) ::TraceSpan MT_UNIQUE_(span_, __LINE__){id}
#else
#  define TRACE_SPAN(id) do {} while (0)     // zero cost, not even a branch
#endif
```
Design decisions and why each one is required by the 1% budget:
- **`rdtsc`/`cntvct_el0`, not `clock_gettime`**: ~20 cycles vs. ~20-25 ns (and a possible
  vDSO/syscall). Calibrate the tick→ns ratio **once** at startup and convert **offline**,
  never per event. Caveat to mention: TSC invariance (`constant_tsc`/`nonstop_tsc`),
  and that it is not synchronized across sockets.
- **Per-thread ring buffer, plain (non-atomic) write index**: no locks, no atomics, no
  contention, no false sharing (hence `alignas`). A shared queue would put an atomic RMW
  on a contended line in the hot path — 100-200 cycles and unbounded under contention.
- **Overwrite-oldest, fixed capacity**: bounded memory and a bounded worst case. Losing
  old events is the right trade; blocking or allocating is not. Keep a dropped-events
  counter so the data is honest.
- **Drain off the hot path**: a low-priority thread (or a per-frame flush at a known
  safe point) copies snapshots out and aggregates into histograms. Aggregation, string
  formatting, and I/O never happen in the measured thread.
- **IDs, not strings**: `stage_id` is a `std::uint32_t` interned at startup; formatting a
  name per event would dominate everything.
- **Compile-time disable** so a release build with tracing off has literally no
  instructions, and a runtime level check (`if (level < threshold) return;` on a
  `constinit` global) for cheap dynamic filtering.
- **Histograms, not raw events, for the metrics**: log-bucketed (HdrHistogram) so p99.9
  is exact and the memory is bounded; keep the raw ring only for dumping the tail when a
  deadline is missed (the technique from section 13 B8).
- **Verify the overhead**, do not assume it: measure the loop with tracing on and off,
  and assert in CI that the delta is under budget. Also mention the ordering hazard —
  `rdtsc` is not a serializing instruction, so out-of-order execution can move it;
  `rdtscp`/an `isb` on ARM fixes that at the cost of a few more cycles, and for a span of
  microseconds it does not matter.
