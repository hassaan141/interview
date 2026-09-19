# 05 — Latency, Throughput, and Benchmarking

> "developing system tools to benchmark, characterize and optimize the latency and
> throughput of the autonomy workloads"

This folder is the job description. If you are strong anywhere, be strong here.

---

## Questions

### Definitions and laws
1. Latency, throughput, bandwidth — define each, and which one does pipelining hurt?
2. State Amdahl's law. A 20% hotspot made infinitely fast gives what speedup?
3. State Little's law and give an autonomy example.
4. Strong vs. weak scaling.
5. What is arithmetic intensity and what do you do with it?
6. Why is the mean latency a misleading metric? What do you report instead?
7. What is coordinated omission?
8. What is tail latency amplification in a pipeline of stages?

### Doing it right
9. Why must you never benchmark at `-O0`?
10. Name four ways to get a microbenchmark wrong.
11. How do you stop the compiler deleting the code you are measuring?
12. How do you make a measurement reproducible on a Linux box?
13. Why interleave A and B rather than running all of A then all of B?
14. How many iterations, and how do you decide?
15. What does a bimodal latency distribution tell you?
16. How would you benchmark something that runs for 10 ms, once?

### Tools
17. What is the first command you run, and which three numbers do you read?
18. IPC 0.35, cache-miss 42%, branch-miss 1% — diagnosis?
19. IPC 0.8, branch-miss 18%, cache-miss 2% — diagnosis?
20. IPC 2.8, low misses, still missing the deadline — diagnosis?
21. What does `perf c2c` find? `perf sched latency`?
22. `stalled-cycles-frontend` vs `-backend`?
23. When is `valgrind --tool=cachegrind` better than `perf`?
24. How do you find out why a loop did not vectorize?
25. What is a flame graph and what question does it answer?

### Building the tooling
26. Design a latency histogram. Why not just store the samples?
27. How do you instrument a 100 Hz pipeline with under 1% overhead?
28. How do you make a benchmark a CI gate without it being flaky?
29. What is PGO and what does it typically buy?
30. You are asked to "characterize" a workload. What do you produce?

---
---

# Answers

**1.** **Latency** = time for one operation. **Throughput** = operations per second.
**Bandwidth** = bytes per second. **Pipelining improves throughput and makes end-to-end
latency worse** (you add handoff, queueing and cache effects). For autonomy the
safety-relevant number is end-to-end sensor→actuation latency, so you pipeline only as far
as the latency budget allows.

**2.** `speedup = 1 / ((1-p) + p/s)`. For p = 0.20 and s = ∞ that is `1/0.8` = **1.25x**.
The corollary is the real lesson: profile first, because the ceiling on optimizing a small
fraction is small no matter how good you are.

**3.** `L = λ × W` — items in the system equals arrival rate times time in system. A 5 ms
stage at 200 frames/s needs `200 × 0.005 = 1` frame in flight; at 8 ms it needs 1.6, so with
one worker the queue grows without bound. It also sizes your ring buffers and thread pools,
and it says throughput and latency are coupled through concurrency.

**4.** **Strong**: fixed problem size, add cores, measure speedup — bounded by Amdahl.
**Weak**: grow the problem with the cores, measure whether time stays constant — Gustafson's
framing, usually the honest one for data-parallel work.

**5.** FLOPs performed per byte moved from memory. Compare `AI × achievable_bandwidth`
against `peak_FLOPS`: if the former is smaller you are **memory-bound** (fix data movement),
if larger you are **compute-bound** (fix instructions). It tells you which optimization
class is worth attempting at all.

**6.** Because a deadline-driven system fails on its **tail**, not its average. A 100 Hz
loop runs 8.6M times a day; a p99.9 of 85 ms means ~8600 misses per day that a 4 ms mean
completely hides. Report p50 / p99 / p99.9 / max, plus the shape of the distribution.

**7.** Measuring only the requests you actually issued, so a stall hides the latency it
causes: if the system freezes for 100 ms, a closed-loop benchmark simply issues fewer
requests and records none of the waiting. The fix is to record latency against the
**intended** start time of each iteration in a fixed-rate loop, not against when you got
around to starting it. Naming this unprompted is a strong signal.

**8.** In a pipeline of N stages, the end-to-end p99 is **not** the sum of the per-stage
p99s — it is worse than the sum of the medians and grows with N, because any one stage
having a bad moment delays the whole frame. With 10 stages each at 99% "fast", only
~90% of frames are fast end to end. Consequence: reducing the *variance* of every stage
matters more than reducing the mean of the slowest one.

**9.** Because at `-O0` the compiler emits naive code that mirrors the source, so you measure
the code generator's laziness rather than your algorithm — and relative results can invert
completely.

**10.** (1) Letting **dead-code elimination** delete the work. (2) No **warmup**, so you
measure cold i-cache, cold branch predictor, and the CPU frequency ramp. (3) Not **pinning**
the CPU / leaving turbo and frequency scaling on. (4) Reporting the **mean of one run**.
(5) An unrepresentative input (all-zero data compresses and predicts differently). (6)
Running all of A then all of B, so machine drift becomes the "result".

**11.** Use the result in a way the compiler cannot see through, or use
`benchmark::DoNotOptimize(x)` / an empty `asm volatile("" : : "r,m"(x) : "memory")`. Also a
`clobber_memory()` barrier after a loop that writes to a buffer. See the harness in
`../../cpp-course/14-performance-architecture-and-optimization/examples.cpp`.

**12.** `taskset -c N` to pin; `cpupower frequency-set -g performance` and disable turbo;
`chrt -f` if you need priority; `isolcpus`/`nohz_full` for the serious version; disable
address-space randomization if it affects layout; run on an otherwise idle machine; and
report the **distribution across repeated runs**, not a single number.

**13.** Because machine state drifts — thermals, frequency, other processes, memory layout.
Interleaving (ABABAB) makes the drift affect both arms equally, so the *difference* stays
meaningful even when the absolute numbers move.

**14.** Enough that the total measured time is far above the clock's resolution and the
per-iteration overhead (typically ≥ 100 ms of work), and enough repetitions to see the
**variance** — at least 10-30 independent runs so you can report a percentile rather than a
point. Then check that the variance is small relative to the effect you are claiming; if it
is not, you have not measured anything.

**15.** That there are **two distinct code paths or system states** — a fast path and a slow
one. Candidates: a cache hit vs. miss, a branch predicted vs. mispredicted, an allocation
served from the free list vs. the kernel, a lock uncontended vs. contended, a rehash or
vector growth, or a thread migrating between core types. The value of the histogram is that
the *shape* tells you where to look; a mean would show one meaningless number in between.

**16.** You cannot microbenchmark it in a loop, because the second iteration has a warm
cache and the first does not — and if it genuinely runs once, the cold state **is** the
thing you care about. Options: measure the real thing many times with a cold cache each time
(flush between runs), use `perf stat` on single executions and aggregate across runs, or
instrument it in production and collect a distribution over real invocations. Say which
question you are answering: steady-state throughput and cold-start latency are different
metrics.

**17.** `perf stat ./app`. Read **IPC** (< 1 means stalling), **cache-misses / references**
(memory bound), and **branch-misses / branches** (unpredictable control flow). Then split
frontend vs. backend stalls.

**18.** **Memory bound.** High miss rate with low branch misses means waiting on DRAM. Look
at the access pattern and layout: pointer chasing, random gathers, or reading one field of a
large struct. Fixes: SoA, smaller types, tiling, removing indirection — then prefetching
only if the pattern is predictable by you but not the hardware.

**19.** **Branch bound.** An 18% mispredict rate means a data-dependent condition that is
essentially random. Fixes: branchless arithmetic on the predicate, sorting or partitioning
the input so the branch becomes predictable, a lookup table, SIMD with masks, or hoisting
the condition out of the loop. `[[likely]]` does **not** help an unpredictable branch — it
helps a biased one.

**20.** The microarchitecture is fine, so either you are **doing too much work** (wrong
algorithm, redundant computation, too much resolution) or you are **not running** when you
should be. Check the second first: `perf sched latency` for wake-up delay, scheduling policy
and affinity, whether another process shares the core, and whether you are blocking on I/O,
a lock, or an allocation.

**21.** `perf c2c` finds **false and true sharing** — it samples HITM events and reports the
exact cache lines and byte offsets being contended, and by which threads. `perf sched
latency` reports per-task **scheduler wake-up delay**, which is how you prove a missed
deadline was jitter rather than compute.

**22.** **Frontend** stalls mean the core could not *fetch or decode* instructions —
i-cache misses, ITLB misses, branch mispredicts, or a code footprint problem (which PGO and
BOLT address). **Backend** stalls mean it could not *execute* — waiting on data cache/DRAM,
or on a saturated execution port. The split immediately tells you whether to fix code layout
or data layout.

**23.** When you need **exact, deterministic** counts rather than samples — a small kernel
where sampling noise exceeds the effect, or a reproducible number for a CI gate, or when you
lack PMU access (containers and VMs often do — as in this environment). The cost is a
20-50x slowdown and a simulated cache model rather than the real one.

**24.** `-fopt-info-vec-missed` (gcc) or `-Rpass-missed=loop-vectorize` (clang) — they name
the loop and the reason. Then check the asm for `ymm`/`zmm` registers. The usual causes:
pointer **aliasing** (fix with `__restrict`), a loop-carried dependency, a non-inlined call
in the body, non-unit stride, an unknown trip count, or FP reassociation not being permitted.

**25.** A stacked visualization of sampled call stacks: width is time attributed, and the
y-axis is stack depth. It answers **"where is the time going, across the whole call tree"**
— much better than a flat profile for finding that a cost is spread across many call sites
of one function. It does **not** answer "why is this slow" (that is `perf stat` counters)
and it hides tail events because it aggregates.

**26.** Log-bucketed counters (HdrHistogram-style): the bucket index comes from the value's
bit width plus a few mantissa bits, giving constant *relative* precision across many orders
of magnitude in a few kilobytes. Not storing samples because that is O(n) memory (3.6M
samples per stage per hour at 1 kHz) and needs a sort to get p99; a histogram is fixed
memory, O(1) per record, and **merges across threads by adding counts** — so each thread
keeps its own and there is no contention. Implementation:
`../../leetcode/17-design-and-systems-questions/07-latency-histogram.cpp`.

**27.** A budget of 1% of a 10 ms period is 100 µs per frame — enormous for tracing, so the
constraints are really about **determinism**, not average cost. Use: a monotonic hardware
counter (`rdtsc`/`cntvct_el0`, ~20 cycles) rather than `clock_gettime`; a **per-thread
lock-free ring buffer** of fixed-size records so there is no allocation, no lock and no
false sharing; interned integer IDs rather than strings; aggregation into histograms off the
hot path by a low-priority drain thread; and a compile-time switch that removes the
instrumentation entirely. Then **measure the overhead** and assert it in CI rather than
assuming it.

**28.** Run it on a **dedicated, pinned, frequency-locked** runner; take the **median of N
repetitions** and gate on a percentile, not a single sample; gate on a threshold well above
the measured run-to-run noise (measure the noise first, then set the threshold); compare
against a stored baseline from the merge base rather than an absolute number; and track the
time series so you catch slow drift that no single commit trips. If the noise is too high to
gate, publish the trend and alert on the 7-day slope instead — a flaky gate gets disabled,
which is worse than no gate.

**29.** Profile-Guided Optimization: build instrumented, run a representative workload, then
rebuild using the profile. The compiler gets real branch probabilities and call frequencies,
so it lays out hot paths contiguously, inlines the calls that matter, and moves cold code
out of line. Typically **5-20%**, more on branchy code. AutoFDO does the same from a `perf`
profile of a production binary, avoiding the instrumented build. BOLT goes further and
re-lays-out the linked binary for i-cache locality, for another 5-15%.

**30.** A characterization is not a single number. Produce: (1) the **latency
distribution** per stage and end to end — p50/p99/p99.9/max, with the histogram, not just
the summary; (2) **throughput** and where it saturates; (3) the **roofline position** —
arithmetic intensity, achieved bandwidth vs. peak, achieved FLOPS vs. peak, so the reader
knows whether it is memory or compute bound; (4) the **microarchitectural counters** — IPC,
cache and branch miss rates, stall breakdown; (5) **scaling behaviour** with cores and with
input size; (6) **resource use** — CPU, accelerator occupancy, memory high-water mark,
allocation counts, bandwidth; (7) the **sensitivity**: which inputs or conditions produce the
tail; and (8) a short written **conclusion naming the bottleneck and the next action**. The
last item is what makes it useful to someone else, and it is the difference between a
measurement and an analysis.
