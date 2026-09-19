# 04 — Computer Architecture and the FSD Chip

The posting says "the Full-Self-Driving chip" explicitly. You are not expected to know
Tesla's internal silicon details — but you **are** expected to reason correctly about a
custom ARM-based SoC with a neural accelerator, and to know the numbers.

---

## Questions

### The numbers (know these cold)
1. L1 / L2 / L3 / DRAM latency, and the DRAM-to-L1 ratio.
2. Cache line size. Why does it matter?
3. Branch mispredict cost? Context switch? Thread creation?
4. Atomic RMW: line in local L1 vs. owned by another core?
5. How many floats per AVX2 register? Per NEON register?
6. Typical IPC of a modern core. What does IPC < 1 mean?

### Caches and memory
7. Direct-mapped vs. set-associative vs. fully associative.
8. What is cache associativity conflict, and what is "cache thrashing" by stride?
9. Write-through vs. write-back? What is a write-allocate policy?
10. What does the hardware prefetcher handle, and what defeats it?
11. What is a TLB miss and how expensive is it?
12. What is NUMA and what do you do about it?
13. What is MESI? What is a HITM event?
14. What is false sharing and how do you find it?

### CPU internals
15. In-order vs. out-of-order execution. What does OoO hide, and what does it not?
16. What is the reorder buffer? What limits ILP?
17. Why does a single floating-point accumulator run at a quarter speed?
18. What is speculative execution and how does a mispredict get rolled back?
19. What is a store buffer, and how does it relate to memory ordering?
20. RISC vs. CISC — what actually differs today, and why does it matter for ARM?

### ARM specifics
21. Three ways aarch64 differs from x86-64 that affect your C++.
22. What is the weak memory model, concretely? Which C++ feature protects you?
23. What is `dmb ish` and when does the compiler emit one?
24. What is `big.LITTLE` and what does it do to your benchmarks?

### Accelerators and SoCs
25. What is a DSP/NPU/TPU-style accelerator good at, and what is it bad at?
26. What is SIMT and how does divergence hurt?
27. What is DMA and why does it matter for a sensor pipeline?
28. What is the roofline model and how do you use it?
29. Why are most perception kernels memory-bound rather than compute-bound?
30. Given a custom inference accelerator on the same die, what would you measure first?

---
---

# Answers

**1.** L1 ~1 ns (4 cycles), L2 ~4 ns (12 cycles), L3 ~15-40 ns, DRAM ~80-100 ns. **DRAM is
roughly 100x an L1 hit** — the single most important ratio in performance work.

**2.** **64 bytes** (128 on Apple M-series and some server ARM;
`std::hardware_destructive_interference_size` tells you). It matters because it is the unit
of transfer and of coherence: reading one byte fetches 64, and two threads writing
different bytes in one line contend.

**3.** Mispredict ~15-20 cycles (the pipeline depth). Context switch ~1-5 µs. Thread
creation ~10-30 µs. Minor page fault ~1-3 µs.

**4.** Local L1, uncontended: ~5-20 cycles. Line owned by another core: the coherence
protocol must transfer exclusive ownership — ~100-200 cycles, and throughput collapses
under contention.

**5.** AVX2 is 256-bit → **8 floats** (4 doubles); AVX-512 → 16. NEON is 128-bit → **4
floats** (2 doubles). So `double`→`float` both halves memory traffic and doubles the vector
width.

**6.** A modern wide core retires **4-6** instructions per cycle. IPC < 1 means it is
stalling most cycles — narrow it down with `stalled-cycles-frontend` (i-cache/mispredicts)
vs. `stalled-cycles-backend` (data/memory).

**7.** **Direct-mapped**: each address maps to exactly one line — cheap, but two hot
addresses with the same index evict each other forever. **Fully associative**: any address
in any line — no conflicts, but the lookup is expensive, so it is used only for small
structures like the TLB. **Set-associative** (N-way) is the compromise: an address maps to a
set of N lines. Real L1s are typically 8-way, L2/L3 12-16-way.

**8.** With N-way associativity, N+1 hot addresses that map to the same **set** evict each
other even though the cache is mostly empty. Classic trigger: striding through a 2D array
with a power-of-two row size, so every row lands in the same set. Fix by **padding** the row
stride to a non-power-of-two (e.g. 1024+8 floats), which is a genuinely surprising 2-10x.

**9.** **Write-through** propagates every write to the next level immediately (simple,
higher traffic); **write-back** marks the line dirty and writes it out on eviction (less
traffic, standard for L1/L2). **Write-allocate** fetches the line on a write miss — which
is why writing a whole array you never read still reads it from memory, and why
non-temporal stores (`_mm_stream_ps`) exist to bypass that.

**10.** It handles sequential access (forward or backward) and constant strides within a
page, for a limited number of concurrent streams. It **cannot** follow a pointer chase
(each address depends on the previous load), an indirect/gather pattern, or a random
permutation — measured at ~32x in
`../../cpp-course/14-performance-architecture-and-optimization/examples.cpp`.

**11.** The TLB caches virtual→physical translations; a miss requires a page-table walk —
several dependent memory accesses, potentially tens to hundreds of cycles, and it can itself
miss cache. With 4 KB pages a 1 GB working set needs 262k translations against a ~1500-entry
TLB, so it thrashes. Huge pages are the fix.

**12.** Non-Uniform Memory Access: on a multi-socket machine, memory attached to another
socket costs ~2x the latency and shares an interconnect. Fix by pinning threads
(`numactl --cpunodebind`) and relying on **first-touch** allocation — the page is placed on
the node of the thread that first writes it, so initialize data in the thread that will use
it, not in a single startup thread.

**13.** MESI is the cache coherence protocol: each line is Modified, Exclusive, Shared, or
Invalid. A **HITM** ("hit modified") is a read or write that finds the line Modified in
another core's cache, forcing a transfer — it is the direct signature of true or false
sharing, and it is what `perf c2c` counts.

**14.** Two threads writing different variables in the same 64-byte line, so the line
ping-pongs. Find it with **`perf c2c record/report`**, which names the line and the byte
offsets, or by A/B testing with `alignas(64)` padding. Measured at ~5x in
`../../cpp-course/11-concurrency-and-the-memory-model/examples.cpp`.

**15.** **In-order** stalls the whole pipeline on any unready operand; **out-of-order**
executes independent instructions past a stall, using register renaming and a reorder buffer
to retire in program order. OoO hides **latency** (a cache miss can overlap with other
work) but not **bandwidth** or a long dependency chain — you cannot execute past a stall if
everything depends on it.

**16.** The ROB holds in-flight instructions so they can retire in program order despite
executing out of order. ILP is limited by true data dependencies (a chain), by the ROB and
scheduler size, by execution-port conflicts, and by branch mispredicts flushing the window.

**17.** Because FP add has ~4 cycles of **latency** but 1-2 per cycle of **throughput**. A
single accumulator creates a serial dependency chain — each add waits for the previous
result — so the unit sits idle 3 of every 4 cycles. Four independent accumulators fill it:
measured at ~4x.

**18.** The core predicts a branch and executes down that path speculatively; results go into
the ROB but are not architecturally committed. On a mispredict the pipeline is flushed and
the speculative entries are discarded. The *microarchitectural* effects (cache state) are
**not** rolled back, which is the basis of Spectre.

**19.** A store buffer holds committed stores before they reach cache, so the core does not
stall on a write. It is exactly why x86's TSO allows **store→load** reordering: a load can
be satisfied from cache while an earlier store is still in the buffer. `mfence` (or a
`seq_cst` store) drains it.

**20.** Today the classic distinction is blurred — x86 decodes CISC instructions into
RISC-like micro-ops. What still differs and matters for you: ARM is **load/store** (only
loads and stores touch memory, so read-modify-write is explicit), has **fixed-width
instructions** (simpler decode, but larger code for constants), has **more general-purpose
registers** (31 vs. 16, so fewer spills), and — the one that will actually bite you — has a
**weaker memory model**.

**21.** (1) **Weak memory ordering** — code with incorrect `relaxed` atomics that works on
x86 breaks on ARM. (2) **`char` is unsigned by default**, so `char c = 0xFF; int i = c;`
gives 255 rather than -1. (3) **Alignment**: unaligned access is allowed for normal loads
but faults for some instructions (exclusive/atomic, and some NEON forms), so a
`reinterpret_cast` to a misaligned `uint64_t*` that silently worked on x86 can trap. Also:
NEON is 128-bit (not 256), `long double` differs, and stack alignment rules differ.

**22.** Concretely: two stores to different addresses can become visible to another core in
the opposite order, and two loads can be satisfied out of order. So the pattern "write data,
then set a ready flag" can be observed as "flag set, data stale". The protection is the C++
memory model: a **release** store paired with an **acquire** load, which makes the compiler
emit the necessary barrier (`stlr`/`ldar` or `dmb ish`).

**23.** A data memory barrier, inner-shareable domain — it orders memory accesses before it
against those after it, across the cores that share coherence. The compiler emits one for
`seq_cst` operations and for `std::atomic_thread_fence`; for acquire/release it usually
prefers the cheaper `ldar`/`stlr` load-acquire and store-release instructions.

**24.** Heterogeneous cores: a few high-performance "big" cores and several
energy-efficient "LITTLE" ones, with the scheduler migrating threads between them. For
benchmarks it is a disaster unless you control it: the same code can be 3x slower depending
on which core it lands on, and a migration mid-measurement adds a cold cache. **Pin to a
specific core**, and say which class it was.

**25.** Good at: dense, regular, highly parallel arithmetic with a fixed dataflow —
convolutions, GEMM, fixed-point/low-precision math — with far better ops-per-watt than a
CPU. Bad at: branchy or irregular control flow, pointer chasing, small batches (the
per-invocation setup dominates), anything requiring fine-grained synchronization with the
CPU, and any operator the hardware does not implement — which then falls back to the CPU
and can dominate the whole inference.

**26.** Single Instruction, Multiple Threads: many lanes execute the same instruction on
different data in lockstep (a warp/wavefront). **Divergence** is when lanes take different
branches — the hardware must execute both paths with the inactive lanes masked off, so a
2-way branch inside a warp costs 2x. Hence branch-free kernels and sorting work by branch
outcome.

**27.** Direct Memory Access: a peripheral moves data to or from memory **without the CPU
copying it**. It matters because a camera at 30 fps × several megapixels would otherwise
burn a core on `memcpy` alone and add latency. The consequences for your code: the buffer
must be physically contiguous and correctly aligned, you must handle **cache coherence**
(invalidate before reading a DMA-written buffer, flush before a DMA read, on non-coherent
platforms), and the completion arrives as an interrupt — so the buffer's lifetime is
controlled by hardware, not by a destructor.

**28.** Plot attainable performance against **arithmetic intensity** (FLOPs per byte moved).
The roof is `min(peak_FLOPS, AI × peak_bandwidth)`. Measure your kernel's AI and its
achieved FLOPS: if you are under the slanted part you are **memory-bound** (optimize data
movement — layout, tiling, precision); under the flat part you are **compute-bound**
(optimize instructions — SIMD, FMA, a better algorithm). It tells you which class of
optimization is even worth attempting.

**29.** Because they stream large images or point clouds and do only a few operations per
byte. A 3x3 convolution on `float32` does ~18 flops per 4-byte element read (AI ≈ 4.5),
while a modern core can do ~50-100 flops per byte of bandwidth — so you are bandwidth-bound
by an order of magnitude. That is why layout (SoA, smaller types, tiling, fusing passes to
avoid round-trips to DRAM) beats instruction-level tricks in perception code.

**30.** In order: (1) **End-to-end latency**, not just kernel time — including the host-side
preprocessing, the DMA transfer in and out, and the synchronization, because those often
exceed the compute. (2) **Accelerator occupancy/utilization** — is it idle waiting for data?
(3) **Which operators actually ran on the accelerator** versus falling back to the CPU; one
unsupported op in the middle of a graph can force two extra round trips and dominate.
(4) **Memory bandwidth** consumed, against the roofline, to know whether you are compute or
bandwidth limited. (5) **Tail latency and jitter**, p99.9 not mean, because that is what a
deadline cares about. (6) **Power and thermal headroom**, since sustained throughput on an
automotive SoC is often thermally limited rather than architecturally limited — a benchmark
that runs for 10 seconds tells you nothing about the steady state.
