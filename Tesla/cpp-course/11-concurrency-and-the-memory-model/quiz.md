# 11 — Mock interview questions

## A. Rapid fire

1. Define a data race precisely. What is the consequence in the standard?
2. What happens if a `std::thread` is destroyed without join or detach?
3. Why is `std::jthread` the right default in C++20?
4. What are the four Coffman conditions for deadlock? How do you break each?
5. Why does `std::scoped_lock` prevent the classic two-mutex deadlock?
6. Why must you never call a callback while holding a lock?
7. Why must `cv.wait` always use the predicate form? Name the two failure modes.
8. Should you `notify` inside or outside the lock? Why?
9. `notify_one` vs `notify_all` — when must you use `notify_all`?
10. Cost of an uncontended mutex lock? A contended one?
11. When is `std::atomic<T>` not lock-free, and how do you check?
12. Why does `compare_exchange_weak` exist, and where must you use it?
13. What does `fetch_add` return?
14. Explain release/acquire in terms of happens-before, in two sentences.
15. When is `memory_order_relaxed` correct? Give two real examples.
16. Why is `seq_cst` the default? What does it cost on x86? On ARM?
17. Why is `volatile bool stop_flag` wrong for stopping a thread? Give three reasons.
18. What is false sharing? How do you detect it? How do you fix it?
19. Why is `std::shared_mutex` often *slower* than `std::mutex`?
20. Two surprises in `std::async`.
21. Why is x86 testing insufficient for ARM concurrency correctness?
22. Which sanitizer finds races, and what is its slowdown?
23. What OS-level knobs matter more than any C++ feature for a real-time loop?

## B. Find the bug

**B1.**
```cpp
std::vector<std::thread> workers;
for (int i = 0; i < 4; ++i) workers.emplace_back([i] { work(i); });
// function returns
```

**B2.**
```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;
// waiter:
std::unique_lock lk{m};
cv.wait(lk);
process();
```

**B3.**
```cpp
void transfer(Account& a, Account& b, int n) {
    std::lock_guard la{a.m};
    std::lock_guard lb{b.m};
    a.bal -= n; b.bal += n;
}
```

**B4.**
```cpp
class EventBus {
    std::mutex m_;
    std::vector<std::function<void()>> subs_;
public:
    void publish() { std::lock_guard lk{m_}; for (auto& s : subs_) s(); }
    void subscribe(std::function<void()> f) { std::lock_guard lk{m_}; subs_.push_back(f); }
};
```

**B5.**
```cpp
std::atomic<bool> ready{false};
Frame frame;
// producer
frame = build();
ready.store(true, std::memory_order_relaxed);
// consumer
while (!ready.load(std::memory_order_relaxed)) {}
use(frame);
```

**B6.**
```cpp
struct Stats {
    std::atomic<std::uint64_t> frames_processed{0};
    std::atomic<std::uint64_t> frames_dropped{0};
};
// 8 threads, each incrementing one of the two
```

**B7.**
```cpp
int counter = 0;
std::mutex m;
void inc() { m.lock(); ++counter; if (counter > 100) return; m.unlock(); }
```

**B8.**
```cpp
std::async(std::launch::async, [] { long_task(); });
do_other_work();
```

**B9.**
```cpp
std::shared_ptr<Config> g_config;
void reload() { g_config = load(); }         // one writer thread
void use()    { auto c = g_config; use(*c); } // many reader threads
```

**B10.**
```cpp
struct Packed { std::uint32_t a : 16; std::uint32_t b : 16; };
Packed p;
// thread 1 writes p.a, thread 2 writes p.b
```

## C. Whiteboard

**C1.** Implement a single-producer single-consumer lock-free ring buffer with the
correct memory orderings. State exactly why each ordering is what it is.

**C2.** Implement a thread pool: fixed worker count, a task queue, `submit` returning
a `std::future`, and clean shutdown. Then say what you would change for a
hard-real-time path.

**C3.** A perception pipeline has 4 stages at 30 Hz. Frame N's stage 2 must not start
before frame N's stage 1 finishes, but stage 1 of frame N+1 may run concurrently with
stage 2 of frame N. Design the synchronization and state the latency vs. throughput
trade-off.

**C4.** You are told "the planner misses its 10 ms deadline about once a minute".
Walk through your diagnosis.

---
---

# Answers

**A1.** Two or more threads access the **same memory location**, at least one of them
writes, and there is no *happens-before* relationship ordering the accesses. The
consequence is **undefined behavior** — not a stale or torn value, UB: the compiler
may have hoisted the load out of a loop, kept the variable in a register forever, or
deleted a branch. Note "memory location": two distinct bitfields in one allocation
unit count as the same location.

**A2.** `std::terminate` is called from `~thread`. This is deliberate — silently
detaching would leave a thread referencing a destroyed stack frame, and silently
joining would make a destructor block for an unbounded time.

**A3.** Its destructor calls `request_stop()` then `join()`, so it is RAII-correct on
every exit path including an exception, and it carries a `std::stop_token` for
cooperative cancellation so you do not have to hand-roll an `atomic<bool>` and thread
it through.

**A4.** (1) **Mutual exclusion** — break it with lock-free structures or immutable
data. (2) **Hold and wait** — acquire everything at once (`std::scoped_lock`,
`std::lock`) or nothing. (3) **No preemption** — use `try_lock` with backoff, or
timed locks. (4) **Circular wait** — impose a **global lock ordering** (by address,
by a level number, by a documented hierarchy). In practice: global ordering plus
`scoped_lock`.

**A5.** It uses `std::lock`, which performs a try-and-back-off algorithm: it locks
one, tries the others, and if any `try_lock` fails it releases everything and retries
starting from the one that blocked. So no thread ever *holds* one lock while blocking
on another — the hold-and-wait condition is eliminated regardless of the order the
caller wrote.

**A6.** Because the callback is code you do not control: it may block, take another
lock (→ deadlock with a different ordering), re-enter your object (→ deadlock on a
non-recursive mutex, or a broken invariant), run for an unbounded time (→ latency), or
throw. Copy the callback list under the lock, release, then invoke.

**A7.** Because of **spurious wakeups** (`wait` may return with no notification at
all — permitted by the standard and real on POSIX) and **lost wakeups** (a `notify`
issued before the waiter reached `wait` is gone forever, since a condition variable
has no memory). The predicate form re-checks the condition on every wake and checks it
*before* first waiting, which fixes both.

**A8.** Either is correct. **Outside** is usually better: if you notify while holding
the lock, the woken thread immediately blocks on the mutex you still hold (the
"hurry up and wait" problem), costing an extra context switch. The exception is when
the notifying thread might destroy the condition variable or the predicate's data
right after — then notify inside, or use another mechanism.

**A9.** Use `notify_all` when more than one waiter could make progress, when waiters
wait on **different predicates** on the same cv (otherwise you can wake the wrong one
and lose the notification), and on shutdown/close. `notify_one` is an optimization
that is only safe when any single waiter is equivalent and can consume the event.

**A10.** Uncontended: ~20 ns — a single atomic CAS in userspace, no syscall (a glibc
futex only enters the kernel on contention). Contended: a `futex` syscall plus a
context switch, ~1-10 µs, and it destroys cache locality. That ~100x gap is the whole
reason to keep critical sections short.

**A11.** When `sizeof(T)` exceeds what the hardware can do atomically (typically >8
bytes, or >16 with `cmpxchg16b`/`casp`), or when `T` is not suitably aligned.
`std::atomic<T>` then embeds a mutex or uses a lock table — so it still *works*, but
it is no longer signal-safe or usable in a lock-free algorithm, and it is much slower.
Check with `std::atomic<T>::is_always_lock_free` (a compile-time constant, so you can
`static_assert` it) or `a.is_lock_free()` at runtime.

**A12.** On LL/SC architectures (ARM, RISC-V, POWER) the store-conditional can fail
for reasons unrelated to the compared value — an interrupt, a cache-line eviction, or
another core touching the line. `compare_exchange_weak` exposes that so the
implementation does not have to hide it behind an internal retry loop, making it
cheaper. Use `weak` **inside a loop** you were going to write anyway, and `strong`
when you are not looping.

**A13.** The **previous** value (like `x++`). All the `fetch_*` functions return the
value before the operation. `operator++` on an atomic returns the *new* value for
prefix and the old for postfix, matching the non-atomic semantics.

**A14.** A **release** store and an **acquire** load on the same atomic form a
synchronization edge: if the acquire load reads the value written by the release
store, then everything sequenced before the release store in the writing thread
*happens-before* everything sequenced after the acquire load in the reading thread.
So all the plain writes you did before publishing the flag are guaranteed visible to
anyone who sees the flag.

**A15.** When you need atomicity (no torn values, no UB) but no ordering relative to
other memory operations. Real examples: (1) a statistics/telemetry counter that is
only read after all threads have joined; (2) a reference-count **increment** (the
final decrement still needs `acq_rel`, because the thread that destroys the object
must see every prior write); (3) a "has anyone ever set this" flag where the only
requirement is eventual visibility and the reader re-validates independently.

**A16.** It is the default because it is the only ordering that composes intuitively —
all `seq_cst` operations appear in one global total order consistent with program
order, which makes algorithms like Dekker's and the IRIW litmus test behave the way
people expect. Cost: on x86-64 a `seq_cst` **store** needs an `xchg` or
`mov`+`mfence` (~20-40 cycles), while loads are free (TSO already gives acquire).
On ARM it needs a `dmb ish` full barrier on both sides, so `seq_cst` is meaningfully
more expensive than acquire/release there — which is exactly why the weaker orderings
matter on the target and not on your laptop.

**A17.** (1) It is **not atomic** — no guarantee the access is indivisible, and on a
wider type it can tear. (2) It emits **no fences**, so the *hardware* may still
reorder the flag store before the data stores it was meant to publish, and the reader
can see the flag set with stale data. (3) It is formally a **data race**, i.e. UB, so
the standard gives you nothing and TSan will (correctly) report it. Bonus: it is also
*slower* than a relaxed atomic, because every access must go to memory. Use
`std::atomic<bool>` with release/acquire, or `std::stop_token`.

**A18.** Two threads writing **different** variables that happen to share a 64-byte
cache line: each write acquires the line exclusively, invalidating the other core's
copy, so the line ping-pongs through the coherence protocol despite zero logical
sharing. Detect: `perf c2c record/report` (purpose-built for this),
`perf stat -e cache-misses`, or an A/B test with padding. Fix:
`alignas(std::hardware_destructive_interference_size)` on each hot member, per-thread
accumulators combined at the end, or restructure so each thread writes its own array
slice. The example in `examples.cpp` measures **~5x** on this machine.

**A19.** Because acquiring a shared lock is itself a **write** to the reader counter —
a contended atomic RMW on one cache line, shared by every reader. So N readers still
serialize on that line, and you pay more bookkeeping than a plain mutex. It only wins
when the critical section is long enough to amortize that (rule of thumb: hundreds of
nanoseconds or more) and readers greatly outnumber writers. For short read-mostly
critical sections, prefer a plain mutex, a seqlock, RCU/hazard pointers, or an
immutable snapshot published atomically.

**A20.** (1) Without an explicit launch policy, the implementation may choose
`std::launch::deferred`, meaning the task does **not** run until you call `get()` —
so "fire and forget" silently never runs. (2) The returned `std::future`'s
**destructor blocks** until the task completes (unique to futures from `std::async`),
so discarding the return value turns an async call into a synchronous one. Also:
`std::async` is not a thread pool and may create a thread per call.

**A21.** x86-64 is **TSO** (total store order): the only reordering you can observe is
store→load, and every load already has acquire semantics and every store release
semantics for free. So incorrect `relaxed` code usually *works* on x86. **ARM/aarch64
is weakly ordered** — stores and loads to different addresses can be observed out of
order — so the same code breaks on the target. You must test on the real hardware, use
TSan (which models the C++ model, not x86's), and reason from the standard rather than
from observed behaviour.

**A22.** **ThreadSanitizer** (`-fsanitize=thread`), ~5-15x slowdown and ~5-10x memory.
It detects data races, lock-order inversions (potential deadlocks), and misuse of
thread APIs. It cannot be combined with ASan. It only finds races on code paths that
actually execute, so it needs stress tests. Also useful: `valgrind --tool=helgrind`,
and `-D_GLIBCXX_ASSERTIONS`.

**A23.** CPU **affinity** (pin the thread to a core, and isolate that core with
`isolcpus`/`nohz_full` so the scheduler and timer ticks leave it alone), the
**scheduling policy and priority** (`SCHED_FIFO`/`SCHED_DEADLINE` rather than
`SCHED_OTHER`), **memory locking** (`mlockall` so no page faults), **preallocation**
(no `malloc`, so no unbounded latency), disabling **CPU frequency scaling** and deep
C-states, IRQ affinity away from the RT core, and huge pages to reduce TLB misses.
None of that is in the C++ standard, and all of it matters more than your choice of
memory ordering.

---

**B1.** The `std::thread` objects are destroyed at the end of the function without
`join()` or `detach()` → `std::terminate`. Also, if they were detached, `work(i)`
would outlive whatever the lambda referenced. Use `std::vector<std::jthread>`.

**B2.** A bare `cv.wait(lk)` with no predicate: vulnerable to a spurious wakeup
(proceeds with `ready == false`) and to a lost wakeup (if `notify` happened before
this thread reached `wait`, it blocks forever). Write
`cv.wait(lk, [&]{ return ready; });`.

**B3.** Two locks taken in **argument order**, so `transfer(a,b,...)` and
`transfer(b,a,...)` on two threads deadlock. Fix: `std::scoped_lock lock{a.m, b.m};`
(or a consistent global order, e.g. by `std::addressof`). Also missing a
self-transfer guard (`&a == &b` would double-lock a non-recursive mutex).

**B4.** `publish()` invokes arbitrary subscriber callbacks **while holding `m_`**. A
subscriber that calls `subscribe()` self-deadlocks; one that takes another lock can
deadlock with a different ordering; one that blocks stalls all publishers. Fix: copy
the subscriber list under the lock, release it, then invoke — and note that this
introduces the "a subscriber unsubscribes concurrently" race, which is why real
implementations hand out weak handles or tokens.

**B5.** `relaxed` on both sides gives atomicity but **no ordering**, so the consumer
can observe `ready == true` while `frame`'s writes are not yet visible — a genuine
failure on ARM. Use `release` on the store and `acquire` on the load. (Also the plain
`frame` accesses are only race-free *because* of that release/acquire edge.)

**B6.** The two counters are adjacent, so they share a cache line: 8 threads doing
atomic RMWs on one line means the line ping-pongs and throughput collapses — the
classic **false sharing**. Fix: `alignas(std::hardware_destructive_interference_size)`
on each member, or give each thread its own `Stats` and sum at the end (which also
removes the atomics entirely).

**B7.** The early `return` skips `m.unlock()` → the mutex stays locked forever and
every other thread deadlocks. This is why you never call `lock()`/`unlock()` by hand:
`std::lock_guard lk{m};` releases on every path, including exceptions.

**B8.** The temporary `std::future` returned by `std::async` is destroyed at the end
of the full-expression, and **that destructor blocks** until `long_task()` finishes —
so this is not asynchronous at all, it is a synchronous call with extra steps.
`do_other_work()` never overlaps. Keep the future alive in a named variable (and
`get()` it), or use a thread pool / `std::jthread`.

**B9.** `g_config` is a `std::shared_ptr` object being **written** by one thread while
others **read** it. The refcount is atomic, but the `shared_ptr`'s own two pointers are
not, so the readers can observe a torn pointer/control-block pair → double free or
crash. TSan flags it. Fix: `std::atomic<std::shared_ptr<Config>> g_config;` (C++20),
or a mutex, or an RCU/hazard-pointer scheme.

**B10.** Writing two bitfields in the same allocation unit is a **data race**: the
compiler implements each write as a read-modify-write of the whole 32-bit unit, so
updates are lost. The standard is explicit that adjacent bitfields in one allocation
unit are a single memory location. Fix: separate them with an unnamed zero-width
field (`std::uint32_t : 0;`), use two separate `std::uint16_t` members, or make them
`std::atomic`.

---

**C1.**
```cpp
template <typename T, std::size_t Capacity>      // Capacity must be a power of two
class SpscRing {
    static_assert((Capacity & (Capacity - 1)) == 0, "capacity must be a power of 2");
    static constexpr std::size_t kMask = Capacity - 1;

    // Each index gets its own cache line: the producer writes head_ while the
    // consumer writes tail_, and sharing a line would be textbook false sharing.
    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> head_{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> tail_{0};
    alignas(std::hardware_destructive_interference_size) std::array<T, Capacity> buf_{};
public:
    bool push(const T& v) {                                   // producer thread only
        const auto head = head_.load(std::memory_order_relaxed);   // (1)
        const auto next = (head + 1) & kMask;
        if (next == tail_.load(std::memory_order_acquire)) return false;  // (2) full
        buf_[head] = v;                                              // (3) plain write
        head_.store(next, std::memory_order_release);                // (4) publish
        return true;
    }
    bool pop(T& out) {                                        // consumer thread only
        const auto tail = tail_.load(std::memory_order_relaxed);     // (5)
        if (tail == head_.load(std::memory_order_acquire)) return false;  // (6) empty
        out = buf_[tail];                                            // (7) plain read
        tail_.store((tail + 1) & kMask, std::memory_order_release);   // (8) free the slot
        return true;
    }
};
```
Why each ordering:
- **(1), (5) relaxed**: each index is written by exactly one thread, so that thread's
  own load of it needs no synchronization — program order suffices.
- **(2), (6) acquire**: reading the *other* thread's index. The acquire pairs with the
  other side's release store, giving a happens-before edge — which is what makes the
  plain access at (3)/(7) race-free.
- **(4) release**: publishes the element written at (3). Any consumer that observes
  the new `head_` via its acquire load at (6) is guaranteed to see the element.
- **(8) release**: publishes "this slot is free". The producer's acquire at (2) must
  not reuse the slot before the consumer's read at (7) has completed; the release
  ensures (7) happens-before the producer's subsequent write to that slot.
- **Power-of-two capacity** so the wrap is a mask, not a division.
- One slot is always left empty so `head == tail` unambiguously means empty.
Extras to volunteer: this is only correct for **one** producer and **one** consumer;
`T` should be trivially copyable for the plain assignment to be cheap and for the
whole thing to be usable across a shared-memory boundary; cache the other index
locally to avoid reloading it on every operation; and mention that this is exactly
the structure used to hand frames between an ISR/driver thread and a processing
thread with no locks and a bounded worst case.

**C2.**
```cpp
class ThreadPool {
    std::vector<std::jthread>          workers_;
    std::queue<std::function<void()>>  tasks_;
    std::mutex                         m_;
    std::condition_variable            cv_;
    bool                               stopping_{false};
public:
    explicit ThreadPool(unsigned n = std::thread::hardware_concurrency()) {
        if (n == 0) n = 1;                                   // the hint may be 0
        workers_.reserve(n);
        for (unsigned i = 0; i < n; ++i)
            workers_.emplace_back([this] { run(); });
    }
    ~ThreadPool() {
        { std::lock_guard lk{m_}; stopping_ = true; }
        cv_.notify_all();                                    // ALL, not one
    }                                                        // ~jthread joins

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) {
        using R = std::invoke_result_t<F, Args...>;
        // packaged_task owns the promise; the future is handed back to the caller.
        auto task = std::make_shared<std::packaged_task<R()>>(
            [fn = std::forward<F>(f),
             ... a = std::forward<Args>(args)]() mutable { return fn(a...); });
        std::future<R> fut = task->get_future();
        {
            std::lock_guard lk{m_};
            if (stopping_) throw std::runtime_error("pool is shutting down");
            tasks_.emplace([task] { (*task)(); });
        }
        cv_.notify_one();
        return fut;
    }
private:
    void run() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock lk{m_};
                cv_.wait(lk, [this] { return stopping_ || !tasks_.empty(); });
                if (tasks_.empty()) return;                   // stopping and drained
                job = std::move(tasks_.front());
                tasks_.pop();
            }
            job();                                           // OUTSIDE the lock
        }
    }
};
```
Points: `packaged_task` in a `shared_ptr` because `std::function` requires a copyable
target; run the job **outside** the lock; `notify_all` on shutdown; drain before
exiting so submitted work is not silently dropped (or document that it is);
`hardware_concurrency()` can return 0.

**For a hard-real-time path, change:** no `std::function` (it allocates) — use a
fixed-size task object or a `function_ref`; no `std::queue`/`std::mutex`/cv — use a
preallocated **lock-free MPMC ring** with bounded capacity, so `submit` has a bounded
WCET and never blocks; **pin each worker to a core** and set `SCHED_FIFO`; no
allocation anywhere in steady state (preallocate the task storage pool); no
exceptions (return an error from `submit` when the queue is full — and decide the
overload policy explicitly: drop-oldest, drop-newest, or fail); and per-worker queues
with work stealing only if you have measured that you need it. Also: a real-time
"pool" often should not be a pool at all, but a fixed set of threads each owning one
stage of the pipeline, which makes latency analyzable.

**C3.** This is a **software pipeline**: stage *i* of frame *N* runs concurrently with
stage *i−1* of frame *N+1*.
```
time ──────────────────────────────────────────────────>
F0:  [S1][S2][S3][S4]
F1:      [S1][S2][S3][S4]
F2:          [S1][S2][S3][S4]
```
Design:
- **One thread per stage**, each pinned to its own core, connected by **bounded SPSC
  ring buffers** (C1) carrying frame handles, not frame data. Each ring's
  release/acquire pair gives the required "stage 1 of frame N happens-before stage 2
  of frame N" edge for free — that is the whole synchronization; no mutexes needed.
- **Frame buffers come from a preallocated pool**; a frame handle is refcounted or
  owned by exactly one stage at a time, so there is no allocation and no copying.
- **Bounded queues with an explicit overload policy**: if stage 2 falls behind, you
  must decide — block stage 1 (protects correctness, raises end-to-end latency, and
  eventually drops at the sensor), or drop the oldest frame (bounds latency, loses
  data). For a perception stack the usual answer is *drop, and count the drops as a
  first-class metric*.
- Alternative for a fan-out/fan-in stage: `std::barrier` per frame, or a task graph
  with explicit dependencies (TBB flow graph style). Say why you would not: barriers
  couple the stages' jitter, so the slowest stage's tail latency propagates.

**Trade-off to state explicitly:** pipelining increases **throughput** to
1/max(stage time) but the **end-to-end latency** is the *sum* of all stage times plus
queueing — in fact it gets slightly worse than a serial implementation because of
handoff and cache effects. For autonomy, end-to-end latency (sensor → actuation) is
the safety-relevant number, so you pipeline only up to the point where you still meet
the latency budget, and you measure **per-frame latency percentiles (p99/p99.9), not
the mean**. If latency dominates, parallelize *within* a stage (data parallelism over
the image) instead of across frames.

**C4.** "Misses a 10 ms deadline once a minute" = a **tail latency** problem, so
averages are useless. Diagnosis:
1. **Measure it properly first.** Instrument the loop with
   `std::chrono::steady_clock` (or better, a TSC-based counter) and record a histogram
   (HdrHistogram) of per-iteration latency; capture p50/p99/p99.9/max and, crucially,
   **timestamp the outliers** so you can correlate with other events. Once a minute at
   30 Hz is ~1 in 1800 — you need percentile tooling, not `time`.
2. **Classify the outlier**: is the loop *starting* late (scheduling/jitter) or
   *taking* too long (compute)? Log both wake-up time and duration. This single split
   eliminates half the hypotheses.
3. **If it starts late** — scheduler and OS: is the thread `SCHED_OTHER`? Is the core
   shared with an interrupt, a kernel thread, or another process? Check
   `/proc/interrupts`, `perf sched latency`, `cyclictest` as a baseline, CPU frequency
   scaling and C-state transitions, `nohz_full`/`isolcpus`, and THP compaction
   stalls. Fix: pin + isolate the core, `SCHED_FIFO`, move IRQs away, `mlockall`.
4. **If it takes too long** — find the once-a-minute event:
   - **Allocation**: a `malloc` that hits the OS (`brk`/`mmap`) or a page fault.
     Check with `perf stat -e page-faults,minor-faults` and an allocator counter.
     Once-a-minute smells exactly like a container growing past a threshold.
   - **A lock**: a rare contention window, priority inversion (a low-priority thread
     holding a mutex the RT thread needs — the fix is PI mutexes,
     `PTHREAD_PRIO_INHERIT`), or a logging/IO call inside the critical section.
   - **I/O or logging**: a synchronous write, a flush, a DNS lookup, a filesystem
     stall. Logging is the single most common culprit; make it lock-free and
     off-thread.
   - **A data-dependent slow path**: an unusually crowded scene taking a different
     branch, a `std::map` degenerating, a retry loop. Correlate outliers with input
     characteristics.
   - **GC-like behaviour**: destructor storms when a large structure is freed,
     `shared_ptr` cascades, or a rehash.
5. **Confirm causally, don't guess**: `perf record --call-graph` with a trigger on the
   slow path, `perf trace`/`ftrace` around the outlier window, or a ring-buffer
   tracer that you dump only when a deadline is missed (this is the technique that
   actually works for rare events — instrument always, keep only the tail).
6. **Fix and prove it**: preallocate, move logging off the critical path, remove the
   lock or make it wait-free, pin and isolate. Then add a **CI/soak gate on p99.9**,
   because a tail regression is invisible to a mean-based benchmark.
