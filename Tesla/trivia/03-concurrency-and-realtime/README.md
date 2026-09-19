# 03 — Concurrency and Real-Time

An autonomy stack is a pipeline of threads with deadlines. This folder is half C++ memory
model and half Linux real-time, because in practice you need both.

---

## Questions

### Threads and synchronization
1. Define a data race precisely. What is the consequence in the C++ standard?
2. Race condition vs. data race — are they the same thing?
3. What happens if a `std::thread` is destroyed without join or detach? Why?
4. What are the four Coffman conditions for deadlock, and how do you break each?
5. Why does `std::scoped_lock` prevent the classic two-mutex deadlock?
6. What is priority inversion? What is priority inheritance?
7. Why must you never call user code while holding a lock?
8. Why must `cv.wait` use the predicate form?
9. `notify_one` vs `notify_all` — when must you use `notify_all`?
10. Cost of an uncontended mutex? A contended one?
11. When is `std::shared_mutex` actually worth it?
12. What is a spinlock and when is it correct to use one?

### The memory model
13. Explain release/acquire in terms of happens-before.
14. When is `memory_order_relaxed` correct? Two real examples.
15. What does `seq_cst` cost on x86? On ARM?
16. Why is x86 testing insufficient for ARM correctness?
17. Why is `volatile bool` wrong for stopping a thread? Three reasons.
18. Why does `compare_exchange_weak` exist?
19. What is the ABA problem?
20. When is `std::atomic<T>` not lock-free?

### Real-time
21. What makes a system "real-time"? Hard vs. soft vs. firm?
22. What is WCET and why can you not measure it?
23. Name five things that are forbidden in a hard real-time path.
24. What is jitter and how do you measure it?
25. What is `SCHED_FIFO` vs `SCHED_OTHER` vs `SCHED_DEADLINE`?
26. What does `isolcpus`/`nohz_full` do?
27. Why does a real-time thread call `mlockall`?
28. What is rate-monotonic scheduling? What is the utilization bound?
29. How do you hand data from a 1 kHz thread to a 10 Hz thread?
30. Your 10 ms loop misses its deadline once a minute. Where do you look?

---
---

# Answers

**1.** Two or more threads access the same **memory location**, at least one writes, and
there is no happens-before relationship ordering them. The consequence is **undefined
behavior** — the whole program, not just the value.

**2.** No. A **data race** is the specific UB above. A **race condition** is a logic bug
where the outcome depends on timing — you can have one with perfectly synchronized code
(check-then-act under two separate locks, TOCTOU on a file). Fixing all data races does not
fix all race conditions.

**3.** `std::terminate`. Deliberately: silently detaching would leave a thread referencing a
destroyed frame, and silently joining would make a destructor block for an unbounded time.
Use `std::jthread`.

**4.** Mutual exclusion (use lock-free or immutable data), hold-and-wait (take all locks at
once with `std::scoped_lock`), no preemption (`try_lock` with backoff, or timed locks),
circular wait (a global lock ordering). In practice: global ordering plus `scoped_lock`.

**5.** It uses `std::lock`, which try-locks and backs off, releasing everything if any
acquisition would block. No thread ever *holds* one lock while *blocking* on another, so
hold-and-wait is eliminated regardless of the order the caller wrote.

**6.** A low-priority thread holds a mutex that a high-priority thread needs, and a
medium-priority thread preempts the low one — so the high-priority thread waits on the
medium one indefinitely. (This is what nearly lost Mars Pathfinder.) **Priority
inheritance** temporarily raises the lock holder to the priority of the highest waiter;
enable it with `PTHREAD_PRIO_INHERIT`. `std::mutex` gives you no way to request it, which is
a real limitation for RT code.

**7.** Because it is code you do not control: it may block, take another lock in a different
order (deadlock), re-enter your object (deadlock or a broken invariant), run for an
unbounded time, or throw. Copy what you need under the lock, release, then call.

**8.** Because of **spurious wakeups** (`wait` may return with no notification — permitted
and real) and **lost wakeups** (a `notify` before you started waiting is gone forever). The
predicate form re-checks on every wake and checks before first waiting.

**9.** Use `notify_all` when more than one waiter could make progress, when waiters wait on
**different predicates** on the same cv, and on shutdown. `notify_one` is an optimization
that is only safe when any single waiter can consume the event.

**10.** Uncontended: ~20 ns, entirely in userspace (glibc only enters the kernel on
contention). Contended: a `futex` syscall plus a context switch, **1-10 µs**, and it
destroys cache locality. That ~100x cliff is why critical sections must be short.

**11.** Only when the critical section is long enough (hundreds of nanoseconds or more) to
amortize the reader bookkeeping — acquiring a shared lock is itself a contended atomic
write, so N readers still serialize on one cache line. For short read-mostly sections
prefer a plain mutex, a seqlock, RCU, or an immutable snapshot published atomically.

**12.** A busy-wait loop on an atomic flag. Correct only when the expected wait is shorter
than a context switch (~1 µs), the holder cannot be preempted (pinned, with interrupts
considered), and you are not oversubscribed — otherwise you burn a core waiting for a
thread that is not running. Add `_mm_pause`/`YIELD` in the loop. In user space, prefer a
mutex; in a kernel or a pinned RT thread, a spinlock can be right.

**13.** A **release** store and an **acquire** load on the same atomic form a
synchronization edge: if the acquire load reads the value written by the release store, then
everything sequenced before the store in the writing thread happens-before everything after
the load in the reading thread. That is how you publish data without a lock.

**14.** When you need atomicity but no ordering with respect to other memory. (1) A
statistics counter read only after all threads join. (2) A reference-count **increment**
(the final decrement needs `acq_rel` because the destroying thread must see all prior
writes). (3) A flag whose only requirement is eventual visibility, where the reader
re-validates independently.

**15.** On x86-64 (TSO), loads are already acquire and stores already release, so only a
`seq_cst` **store** costs — an `xchg` or `mov`+`mfence`, ~20-40 cycles. On ARM it needs a
`dmb ish` full barrier on both sides, so `seq_cst` is meaningfully more expensive than
acquire/release there.

**16.** x86 is strongly ordered: the only visible reordering is store→load, so incorrect
`relaxed` code usually *works* on a laptop. ARM/aarch64 is weakly ordered and will reorder
independent loads and stores, so the same code breaks on the target. Test on the real
hardware, use TSan (which models the C++ model, not x86), and reason from the standard.

**17.** (1) It is **not atomic** — no indivisibility guarantee, and wider types can tear.
(2) It emits **no fences**, so the hardware can still reorder the flag store ahead of the
data it was meant to publish. (3) It is formally a **data race** (UB), so the standard
promises nothing and TSan reports it. Bonus: it is also slower than a relaxed atomic.

**18.** On LL/SC architectures (ARM, RISC-V, POWER) the store-conditional can fail for
reasons unrelated to the value — an interrupt, a cache-line eviction, another core touching
the line. `weak` exposes that so the implementation need not hide it behind an internal
retry loop, making it cheaper inside a loop you were writing anyway.

**19.** A CAS succeeds because the value it compares is the same, but the *world* changed in
between: a pointer was freed and a new allocation reused the address, so A→B→A looks like
"unchanged". The result is a lock-free stack popping a freed node. Fixes: a tagged pointer
(a version counter packed alongside), hazard pointers, epoch-based reclamation, or simply
never reusing addresses (index-based structures).

**20.** When `sizeof(T)` exceeds what the hardware can do atomically (typically >8 bytes, or
>16 with `cmpxchg16b`/`casp`), or when the object is under-aligned. `std::atomic<T>` then
uses an internal mutex or a lock table — still correct, but no longer signal-safe or usable
in a lock-free algorithm. Check with `std::atomic<T>::is_always_lock_free`.

**21.** Real-time means **correctness depends on meeting a deadline**, not on being fast.
**Hard**: a missed deadline is a system failure (a brake controller). **Firm**: a late
result is useless but not catastrophic (a dropped perception frame). **Soft**: lateness
degrades quality (a UI). Determinism matters more than throughput — a slower system with a
bounded worst case beats a faster one with an occasional 50 ms stall.

**22.** Worst-Case Execution Time. You cannot *measure* it because measurement only samples
the paths and cache/branch-predictor states you happened to hit; the true worst case may
require a pathological input plus a cold cache plus an interrupt. It is *bounded* by static
analysis over the control-flow graph with a machine model (and by forbidding unbounded
loops, recursion, and dynamic allocation). Measurement gives you a high-water mark and a
distribution, which is what you actually gate on in practice.

**23.** Dynamic allocation (`new`/`malloc`/any container that may grow), unbounded blocking
(a mutex you can wait on indefinitely, I/O, logging to disk), exceptions (unbounded unwind),
page faults (hence `mlockall`), recursion and unbounded loops, and anything that calls into
the kernel without a bound. Also: no `printf` on the hot path, and no `std::shared_ptr`
churn if it contends.

**24.** Jitter is the variation in when a periodic task actually runs versus when it should.
Measure it by recording the **wake-up timestamp** of every cycle and histogramming
`actual_start - expected_start` — separately from the execution duration, because the fixes
are completely different (scheduling vs. compute). `cyclictest` gives you the platform
baseline; anything your application adds is yours.

**25.** `SCHED_OTHER` is the default CFS time-sharing policy — fair, but with no latency
guarantee. `SCHED_FIFO` is a fixed-priority real-time policy: a higher-priority runnable
thread always preempts a lower one and runs until it blocks or yields (so a runaway
FIFO thread can hang the machine — use `RLIMIT_RTTIME`). `SCHED_DEADLINE` (EDF) lets you
declare runtime/period/deadline and the kernel admits or rejects the task, which is the most
principled option when it fits.

**26.** `isolcpus=2,3` removes those CPUs from the scheduler's general load balancing so
nothing runs there unless explicitly pinned. `nohz_full=2,3` stops the periodic timer tick
on them when a single task is running, removing ~1 µs interruptions. Together with moving
IRQ affinity away, they give a core that belongs to your RT thread alone.

**27.** So no page of its address space can be swapped out or reclaimed, which would turn a
memory access into a major page fault of hundreds of microseconds to milliseconds — a
guaranteed missed deadline. Combine with pre-touching the heap and stack so the minor faults
also happen at startup.

**28.** RMS assigns static priorities by frequency: the shortest period gets the highest
priority. For n independent periodic tasks it guarantees schedulability if total utilization
is below `n(2^(1/n) − 1)` — 82.8% for two tasks, converging to **ln 2 ≈ 69.3%**. So a useful
rule of thumb: if your periodic tasks use under ~69% of a core, RMS schedules them; above
that you need to check the specific task set.

**29.** A **lock-free SPSC ring buffer** (or a double buffer / seqlock) so the fast thread
never blocks on the slow one, with an explicit overflow policy — usually "overwrite the
oldest and count the drop", because a 1 kHz producer must not be stalled by a 10 Hz
consumer. If the consumer only needs the latest value, a seqlock or an atomic
publish-a-snapshot is even simpler. What you must not do is share a mutex: the 1 kHz thread
would inherit the 10 Hz thread's latency. See
`../../leetcode/17-design-and-systems-questions/01-ring-buffer-spsc.cpp`.

**30.** Once a minute at 100 Hz is a 1-in-6000 tail event, so averages are useless. First
**split wake-up time from execution duration** — that alone halves the hypothesis space.
If it starts late: scheduling (policy, affinity, an interrupt on your core, frequency/C-state
transitions, a timer tick). If it takes too long: an allocation that hit the kernel, a page
fault, a container growth or rehash, a rare lock contention or priority inversion, a
logging/IO call, or a data-dependent slow path. Instrument with an always-on ring-buffer
tracer that you **dump only when the deadline is missed** — you cannot reproduce a rare
event interactively, so you keep the tail and discard the rest. Then `perf sched latency`,
`perf stat -e page-faults`, and a p99.9 gate in CI so it cannot come back.
