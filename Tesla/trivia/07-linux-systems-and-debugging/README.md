# 07 — Linux Systems and Debugging

The day-to-day substrate. You will not be quizzed on all of it, but being unable to answer
"how would you find out?" is disqualifying for a systems role.

---

## Questions

### Processes, threads, scheduling
1. Process vs. thread — what is shared and what is not?
2. What happens on `fork()`? What is copy-on-write there?
3. What does `exec` do to the address space?
4. What is a zombie process? An orphan?
5. User mode vs. kernel mode. What is the cost of a syscall?
6. What is a context switch and why is it expensive?
7. How do you pin a thread to a core, and why would you?
8. What are `nice`, `SCHED_FIFO`, and `chrt`?

### Memory and files
9. What does `/proc/<pid>/status` tell you? VmRSS vs. VmSize?
10. What is the OOM killer and how do you survive it?
11. What is a file descriptor? What happens on `fork`/`exec`?
12. Blocking vs. non-blocking I/O; `select`/`poll`/`epoll`.
13. What is `mmap` and when is it better than `read`?
14. What is the page cache? What does `fsync` guarantee?

### Signals and IPC
15. What can you safely do in a signal handler?
16. What is `SIGSEGV` vs `SIGBUS` vs `SIGILL` vs `SIGFPE`?
17. Name four IPC mechanisms and when you would pick each.
18. How would you share a large sensor buffer between two processes?

### Debugging
19. Five gdb commands you actually use.
20. How do you debug a crash you cannot reproduce?
21. How do you get a core dump and symbolize it months later?
22. What is `rr` and when do you reach for it?
23. How do you find out what a running process is doing, without restarting it?
24. `strace`, `ltrace`, `perf trace`, `bpftrace` — what is each for?
25. How do you find which file descriptor is leaking?
26. A process's RSS grows 2 MB/hour. How do you find out why?
27. A binary segfaults only in release. Approach?
28. How do you find out which shared library a symbol came from?
29. How do you check whether a binary has debug info, is stripped, or is PIE?
30. A service hangs. Nothing is logged. What do you do, in order?

---
---

# Answers

**1.** A process has its own **address space**, file-descriptor table, and permissions.
Threads within a process **share** the address space, heap, globals, file descriptors, and
signal dispositions; each has its **own** stack, registers, thread-local storage, and
`errno`. Consequence: thread communication is free but needs synchronization; process
isolation costs IPC but contains faults.

**2.** It creates a near-identical child with a copied page table whose pages are marked
**copy-on-write** and read-only: neither process pays a copy until one writes, at which
point the kernel takes a fault and duplicates that page. So `fork` is cheap, but the child
inherits everything — including locks held by threads that do not exist in the child, which
is why `fork` in a multithreaded program is dangerous and why you should `fork`+`exec`
immediately or use `posix_spawn`.

**3.** It **replaces** the entire address space with a new program image: the text, data,
heap, and stack are discarded and rebuilt; the process ID, open file descriptors (unless
marked `FD_CLOEXEC`), and working directory survive. Nothing of your C++ objects persists —
so anything the child needs must be passed via arguments, environment, or an inherited fd.

**4.** A **zombie** has exited but its parent has not `wait`ed, so the exit status is still
held in the process table (it consumes a PID, not memory). An **orphan** is still running
but its parent died; it is re-parented to `init`/`systemd`, which reaps it. A pile of
zombies means a missing `wait`/`SIGCHLD` handler.

**5.** User mode is unprivileged: it cannot touch hardware or protected memory and must
trap into the kernel for those. A syscall costs ~50-200 ns today with `syscall`/`sysret`
(much more after Spectre/Meltdown mitigations), plus the cache and TLB pollution from the
kernel's own working set. That is why `vDSO` exists for `clock_gettime`, and why a hot loop
should never call into the kernel.

**6.** Saving one thread's register state and restoring another's, plus switching the kernel
stack — and, between *processes*, switching the page table, which flushes or tags the TLB.
~1-5 µs, but the real cost is the **cold cache** afterwards: the new thread's working set
has been evicted. That indirect cost usually dwarfs the direct one.

**7.** `pthread_setaffinity_np` / `sched_setaffinity`, or `taskset -c N` for a whole
process. Why: to stop the scheduler migrating the thread (which discards its warm cache and
adds jitter), to keep it off a core handling interrupts, to control NUMA locality, and to
make benchmarks reproducible. Combine with `isolcpus`/`nohz_full` for the serious version.

**8.** `nice` adjusts a `SCHED_OTHER` thread's share of CPU (a hint, not a guarantee).
`SCHED_FIFO` is a fixed-priority **real-time** policy — a runnable higher-priority thread
always preempts a lower one and runs until it blocks. `chrt` is the command-line tool to set
the policy and priority (`chrt -f 80 ./app`). A runaway `SCHED_FIFO` thread can lock up a
core, so set `RLIMIT_RTTIME` or keep a lower-priority escape hatch.

**9.** Per-process memory and thread counts. **VmSize** is the total *virtual* address space
reserved — it includes untouched mappings and overcommit, so it is nearly meaningless as a
memory metric. **VmRSS** is the *resident* set: physical pages actually in RAM, which is what
you monitor. Also useful: `VmHWM` (peak RSS), `Threads`, `voluntary_ctxt_switches` vs.
`nonvoluntary_ctxt_switches` (the latter rising means you are being preempted).

**10.** Linux overcommits, so allocation succeeds even when the memory does not exist; when
it runs out, the OOM killer picks a victim by `oom_score` (roughly, the biggest RSS) and
`SIGKILL`s it. Survive it by: bounding your memory (every cache needs a limit), setting
`oom_score_adj` for critical processes, using cgroup memory limits so a runaway is contained
rather than taking down a neighbour, and — for real-time — `mlockall` plus preallocation so
you never depend on late allocation at all.

**11.** A small integer index into the process's file-descriptor table, pointing at an open
file description (which holds the offset and flags) in the kernel. `fork` **duplicates the
table** so parent and child share the same open file descriptions (and therefore the same
file offset). `exec` keeps them open **unless** `FD_CLOEXEC` is set — which is why you
should open with `O_CLOEXEC` by default, or you leak descriptors into every child.

**12.** **Blocking** I/O parks the thread until data is ready — simple, but one thread per
connection. **Non-blocking** returns `EAGAIN` and you must poll or be notified.
`select` is O(n) per call with a 1024-fd limit; `poll` removes the limit but is still O(n);
**`epoll`** is O(1) per ready event with a kernel-side registered set, which is why it scales
to tens of thousands of descriptors. `io_uring` is the modern successor, with a shared
submission/completion ring that avoids the syscall per operation entirely.

**13.** `mmap` maps a file into the address space so you access it as memory, with the page
cache doing the I/O on demand. Better than `read` for random access to a large file, for
sharing the same pages across processes, and for avoiding a copy into user space. Worse for
small files, for sequential streaming (readahead is simpler and predictable), and because
I/O errors arrive as `SIGBUS` rather than an error return — which is a genuine robustness
problem.

**14.** The kernel's cache of file contents in RAM; reads are served from it and writes land
in it and are flushed later, which is why `write` returning does **not** mean the data is on
disk. `fsync(fd)` forces that file's data **and** metadata to stable storage and returns
only when the device says so; `fdatasync` skips non-essential metadata. Neither guarantees
the *directory entry* is durable — for a new file you must also `fsync` the directory.

**15.** Only **async-signal-safe** functions (`write`, `_exit`, `signal`, a specific POSIX
list) and reads/writes of `volatile sig_atomic_t` or lock-free atomics. **Not** `malloc`,
`printf`, or anything taking a lock — the handler can interrupt the same function mid-lock
and deadlock. The standard pattern is to set a flag (or `write` one byte to a self-pipe /
`eventfd`) and do the real work in the main loop.

**16.** `SIGSEGV`: an invalid memory access — unmapped page, null dereference, a write to
read-only memory, or a stack guard page. `SIGBUS`: the address is valid but the access is
not — misaligned access on strict architectures, or reading past the end of an `mmap`ed
file. `SIGILL`: an illegal instruction — usually a binary built with `-march` the CPU does
not support, or a corrupted function pointer. `SIGFPE`: integer division by zero or
`INT_MIN / -1` (despite the name, usually not floating point, which produces NaN/Inf
instead).

**17.** **Shared memory** (`shm_open` + `mmap`): fastest, zero-copy, for large data — but
you must handle synchronization yourself. **Pipes/FIFOs**: simple byte streams, natural
back-pressure, good for parent/child. **Unix domain sockets**: message boundaries with
`SOCK_SEQPACKET`, credentials passing, and **fd passing** via `SCM_RIGHTS` — the right
choice for a control channel. **Message queues / eventfd / signalfd**: notification with a
counter. Pick shared memory + a lock-free ring for a high-rate sensor path, and a socket for
control and lifecycle.

**18.** Shared memory: `shm_open` or `memfd_create` a region, `mmap` it into both processes,
and lay out a **lock-free ring buffer of fixed-size slots** inside it. Requirements: the
frame struct must be trivially copyable and standard layout with fixed-width types and no
pointers (addresses differ between processes — use **offsets** or indices); use
`std::atomic` with explicit ordering for the indices; version the layout and check it at
attach; and decide the policy for a dead consumer (a sequence number plus a heartbeat, so
the producer does not block forever). Pass the fd over a Unix socket with `SCM_RIGHTS` so
there is no name in the filesystem to race on.

**19.** `bt full` / `thread apply all bt` (every thread's stack — the first thing on a
deadlock core), `break file:line if cond`, `watch expr` (a data breakpoint — the fastest way
to find who corrupts a value), `p` / `x/16xb` for memory, `info locals`, `finish`,
`catch throw`, and `set var` to test a hypothesis without recompiling.

**20.** Instrument rather than reproduce: an **always-on ring-buffer tracer** dumped only
when the failure condition fires, so you keep the tail and discard the rest; ensure core
dumps are collected and symbolized; add assertions *earlier* so the failure is caught closer
to the cause; log the inputs in a replayable format so you can re-run the exact scenario
offline; and correlate occurrences statistically (same machine? same sensor? cold start?).
If you can record it at all, `rr` turns it into a deterministic replay.

**21.** `ulimit -c unlimited` (and check `/proc/sys/kernel/core_pattern`, or use
`coredumpctl` under systemd). To symbolize later: keep the **unstripped** binary, keyed by
its **build-id** (`readelf -n`), or ship `objcopy --only-keep-debug` output alongside with
`--add-gnu-debuglink`. Then `gdb ./app core`. Without archived symbols the dump is useless,
which is why the archiving step belongs in the release pipeline, not in someone's head.

**22.** `rr` records an execution and replays it **deterministically**, including reverse
execution (`reverse-continue`, `reverse-step`) in gdb. Reach for it for anything
nondeterministic, rare, or where you need to run *backwards* from a corrupted value to the
write that caused it — heisenbugs, races (deterministic once recorded), and "who freed
this?". It is the single biggest debugging productivity tool on Linux.

**23.** `gdb -p <pid>` and `thread apply all bt` (it stops the process briefly);
`eu-stack`/`pstack` for a non-intrusive snapshot; `perf top -p <pid>` and `perf record -p`
for where the CPU time goes; `strace -p` / `perf trace -p` for syscalls; `cat
/proc/<pid>/status`, `/wchan`, `/stack`, and `/proc/<pid>/task/*/stat` for what each thread
is blocked on; `ls -l /proc/<pid>/fd` for open descriptors; and `bpftrace` for anything
custom without restarting.

**24.** `strace`: traces **syscalls** via ptrace — very informative, very slow (each call
traps twice), fine for diagnosis, never for production. `ltrace`: traces **library** calls
via PLT interception, fragile with modern hardening. `perf trace`: syscall tracing built on
perf events — far lower overhead than strace. `bpftrace`/eBPF: programmable, in-kernel
filtering and aggregation with minimal overhead, and it can probe arbitrary kernel and user
functions — the right tool for anything on a live production system.

**25.** `ls -l /proc/<pid>/fd | wc -l` over time to confirm the growth, then `ls -l` to see
**what** they point to (sockets, a repeated file, pipes) — that usually identifies the code
path immediately. `lsof -p <pid>` for the same with more detail. To catch the *creation*
site: `bpftrace` on `openat`/`socket` returns, or an LD_PRELOAD shim, or run under valgrind
with `--track-fds=yes`. The C++ fix is always the same: wrap the descriptor in an RAII type
(see `cpp-course/05/quiz.md` C1) so no early return can leak it.

**26.** First distinguish a **leak** from **unbounded growth**: `valgrind
--leak-check=full` reports the former; if it says "still reachable", you have the latter —
a cache with no eviction, an append-only vector, a `shared_ptr` cycle, or an
ever-growing map. Use **heaptrack** or `massif` to attribute allocation growth to call sites
over time, or jemalloc/tcmalloc heap profiles in production. Then bound the structure and
export its size as a metric so the next one is caught in an hour.

**27.** Almost certainly **UB or memory corruption** — a stack buffer overflow smashing a
return address is exactly "release-only crash". Rebuild the *release* configuration (keep
`-O2`) with `-fsanitize=address,undefined -g -fno-omit-frame-pointer` and run the failing
input; add `-fstack-protector-strong` to turn a smash into a clean abort; try `-fwrapv`
(if it fixes it: signed overflow) and `-fno-strict-aliasing` (if it fixes it: an aliasing
violation); use `rr` to run backwards from the crash; and check that `NDEBUG` is not
removing an `assert` that was carrying a side effect.

**28.** `ldd ./app` for the resolved libraries; `nm -DC lib.so | grep symbol` to see which
library **defines** it; `readelf -Ws` for full symbol details including version tags; and
`LD_DEBUG=bindings ./app 2>&1 | grep symbol` to see what the loader **actually bound** at
runtime — which is the one that matters when two libraries define the same symbol and
interposition picks the first. `LD_DEBUG=libs` shows the search order.

**29.** `file ./app` gives the quick answer: "ELF 64-bit LSB **pie** executable ... **with
debug_info** ... **not stripped**" (or "stripped"). More precisely: `readelf -S | grep
debug` for debug sections, `readelf -h | grep Type` (`DYN` = PIE, `EXEC` = non-PIE),
`readelf -n` for the build-id, and `readelf -d` for dynamic dependencies and RPATH.

**30.** In order: (1) Is it **running or blocked**? `top`/`ps` — 100% CPU means a spin or an
infinite loop; 0% means blocked. (2) `gdb -p` and **`thread apply all bt`** — this answers
most hangs immediately: every thread's stack shows you the deadlock or the blocking call.
(3) If it is spinning, `perf top -p` shows where. (4) `strace -p` / `perf trace -p` for a
syscall it is stuck in (a socket read with no timeout is the classic). (5) Check for a
**deadlock**: two threads in `futex_wait` with locks held in opposite orders — and note that
TSan would have caught the lock-order inversion in CI. (6) Check resources: fd exhaustion,
a full disk, a full queue, memory pressure. (7) If it recurs, the fix is not just this
instance: add **timeouts to every blocking call**, a watchdog that dumps all stacks on
detection, and logging that survives the hang (a ring buffer flushed by a separate thread,
not a `printf` that blocks on the same lock).
