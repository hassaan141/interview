# `perf` cheatsheet — the commands you will actually type

If `perf` is not installed in your environment: `sudo apt install linux-tools-generic`
(and a container needs `--cap-add=PERFMON` or `--privileged`, plus
`sysctl kernel.perf_event_paranoid=1`).

Build for profiling. Keep these in your **production** build too:
```bash
g++ -std=c++20 -O2 -march=native -g -fno-omit-frame-pointer app.cpp -o app
```

## 1. Start here, always

```bash
perf stat ./app
```
Read it in this order:

| Counter | What it tells you | Action if bad |
| --- | --- | --- |
| **insn per cycle (IPC)** | < 1.0 → the core is stalling | find out *why* below |
| **cache-misses / cache-references** | > ~10% → memory bound | fix layout: SoA, shrink, tile, pool |
| **branch-misses** | > ~5% of branches → unpredictable control flow | branchless, sort the data, `[[likely]]` |
| **page-faults** | growing during steady state → allocating | preallocate, `mlockall`, huge pages |
| **task-clock / CPU utilization** | < 1.0 CPUs → blocking | find the blocking call |

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,\
LLC-load-misses,branch-instructions,branch-misses,page-faults,\
stalled-cycles-frontend,stalled-cycles-backend ./app
```
`stalled-cycles-frontend` high → instruction fetch / i-cache / mispredicts.
`stalled-cycles-backend` high → data cache / memory / execution-unit pressure.

## 2. Then find where

```bash
perf record -g --call-graph dwarf ./app     # dwarf: accurate with -fomit-frame-pointer
perf record -F 999 -g ./app                 # 999 Hz avoids lock-step with a 1 kHz loop
perf report --sort=dso,symbol
perf report --no-children                   # self time, not cumulative
perf annotate <symbol>                      # per-instruction attribution
perf top                                    # live, for a running service
```

## 3. Specialized

```bash
perf c2c record ./app && perf c2c report     # FALSE SHARING: HITM lines and offsets
perf sched record ./app && perf sched latency   # scheduler wakeup delay (jitter)
perf stat -e 'syscalls:sys_enter_*' ./app    # which syscalls, how often
perf trace ./app                             # strace-like, much cheaper
perf mem record/report                       # load/store latency distribution
perf probe --add 'my_function'               # dynamic tracepoint
perf record -e cache-misses:pp -g ./app      # sample ON cache misses, precise IP
perf stat -I 1000 -p <pid>                   # 1 s interval counters on a live process
```

## 4. Flame graphs

```bash
perf record -F 99 -g -- ./app
perf script | stackcollapse-perf.pl | flamegraph.pl > out.svg
# or: https://github.com/brendangregg/FlameGraph
```

## 5. Making measurements stable (do this first, or nothing reproduces)

```bash
sudo cpupower frequency-set -g performance          # no frequency scaling
echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost   # no turbo
taskset -c 2 ./app                                  # pin to one core
chrt -f 80 ./app                                    # SCHED_FIFO for an RT loop
# kernel cmdline for a real RT setup: isolcpus=2,3 nohz_full=2,3 rcu_nocbs=2,3
sudo sysctl -w kernel.perf_event_paranoid=1
sudo sysctl -w kernel.numa_balancing=0
```

## 6. When `perf` is not the right tool

| Question | Tool |
| --- | --- |
| exact instruction/cache counts, no sampling noise | `valgrind --tool=cachegrind` / `callgrind` |
| who allocated all this memory | `heaptrack`, `valgrind --tool=massif`, jemalloc profiles |
| is this loop's *body* throughput-limited | `llvm-mca`, and read the asm on Compiler Explorer |
| A/B a single function | google-benchmark, quick-bench.com |
| why did the compiler not vectorize | `-fopt-info-vec-missed`, `-Rpass-missed=loop-vectorize` (clang) |
| a nondeterministic bug or a rare tail event | `rr`, or an always-on ring-buffer tracer dumped on the outlier |
| end-to-end latency in a pipeline | your own `TRACE_SPAN` instrumentation + percentile histograms |

## 7. Vectorization diagnostics

```bash
g++   -O3 -march=native -fopt-info-vec -fopt-info-vec-missed -c kernel.cpp
clang -O3 -march=native -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -c kernel.cpp
g++   -O2 -S -masm=intel -o - kernel.cpp | grep -E 'ymm|zmm|xmm'   # did it use SIMD?
```
Top reasons a loop does not vectorize: **pointer aliasing** (fix with `__restrict`),
a loop-carried dependency, a function call in the body (needs inlining), non-unit
stride, an unknown trip count, control flow inside the loop, and FP reassociation not
being allowed (that one is deliberate — see section 01).
