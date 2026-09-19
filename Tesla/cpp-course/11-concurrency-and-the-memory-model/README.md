# 11 — Concurrency: Threads, Mutexes, Atomics, the C++ Memory Model

Course chapters: **22 (Advanced Topics II), concurrency part; 23 (memory ordering)**

An autonomy stack is a pipeline of threads with hard deadlines. Expect at least one
question here, and expect it to go deep on the memory model.

---

## 1. Threads

```cpp
#include <thread>
std::thread t{[]{ work(); }};
t.join();                                  // MUST join or detach before ~thread,
                                           // or std::terminate is called
std::jthread jt{[](std::stop_token st) {   // C++20: joins in its destructor and
    while (!st.stop_requested()) work();   // supports cooperative cancellation
}};
jt.request_stop();                         // ~jthread calls this + join()

std::this_thread::sleep_for(10ms);
std::this_thread::yield();
std::thread::hardware_concurrency();       // a HINT, may return 0
```

`std::jthread` is the default in C++20 and later: RAII join, cancellation via
`std::stop_token`. Use it.

A **data race** is: two threads access the same memory location, at least one
writes, and there is no happens-before relationship between them. It is **undefined
behavior** — not "a wrong value", UB. Note "memory location": distinct bitfields in
the same allocation unit are *one* location (see section 02).

Thread affinity and priority are OS-level, not standard C++:
`pthread_setaffinity_np`, `sched_setscheduler(SCHED_FIFO)`,
`pthread_setschedparam`. For a real-time loop, pinning to a core and using
`SCHED_FIFO` matters more than anything in the language. Say that.

## 2. Mutual exclusion

```cpp
std::mutex m;                       // ~40 bytes, uncontended lock ≈ 20 ns (futex)
std::recursive_mutex rm;            // same thread may lock repeatedly. Usually a
                                    // design smell
std::shared_mutex sm;               // multiple readers OR one writer
std::timed_mutex tm;                // try_lock_for / try_lock_until

{ std::lock_guard lk{m}; }                    // minimal, non-movable
{ std::scoped_lock lk{m1, m2}; }              // C++17: deadlock-free multi-lock
{ std::unique_lock lk{m, std::defer_lock}; }  // movable, try_lock, needed by cv
{ std::shared_lock lk{sm}; }                  // reader lock

std::once_flag f;
std::call_once(f, []{ init(); });              // exactly once, thread safe
// (or just use a function-local static, which does this for you)
```

Deadlock requires four conditions (Coffman): mutual exclusion, hold-and-wait, no
preemption, circular wait. Break any one. Practical rules:
1. **Acquire multiple locks in a single consistent global order** — or use
   `std::scoped_lock`, which applies a deadlock-avoidance algorithm.
2. **Never call user code (a callback, a virtual function) while holding a lock.**
3. Hold the lock for the shortest possible region; never across I/O.
4. Prefer one coarse lock to many fine ones until you have measured.

`std::condition_variable` — the two rules that matter:
```cpp
std::mutex m; std::condition_variable cv; std::queue<Job> q; bool done = false;

// producer
{ std::lock_guard lk{m}; q.push(job); }
cv.notify_one();                              // notify OUTSIDE the lock is fine and
                                              // usually better (no immediate contention)

// consumer
std::unique_lock lk{m};
cv.wait(lk, [&]{ return !q.empty() || done; });   // PREDICATE form: handles
                                                   // spurious AND lost wakeups
```
Always use the predicate overload — a bare `cv.wait(lk)` is wrong because of
**spurious wakeups** (the thread may wake with no notification) and **lost wakeups**
(a notify that arrives before you wait is gone forever). The mutex must be held when
you check the predicate, and `wait` atomically releases and reacquires it.

## 3. Atomics

```cpp
std::atomic<int>  counter{0};
std::atomic<bool> flag{false};
std::atomic<T*>   head{nullptr};

counter.fetch_add(1, std::memory_order_relaxed);     // returns the OLD value
counter.store(5, std::memory_order_release);
int v = counter.load(std::memory_order_acquire);
int expected = 5;
counter.compare_exchange_weak(expected, 6);          // CAS; on failure, `expected`
                                                     // is updated to the actual value
counter.compare_exchange_strong(expected, 6);        // no spurious failure

static_assert(std::atomic<int>::is_always_lock_free);
std::atomic_ref<int> ref{non_atomic_int};             // C++20: atomic view of an object
std::atomic<std::shared_ptr<T>> sp;                   // C++20
flag.wait(false); flag.notify_all();                  // C++20: atomic wait/notify
```
`is_lock_free`: types up to the native word size (and usually 16 bytes with
`cmpxchg16b`) are lock-free; a larger `std::atomic<BigStruct>` silently uses an
internal mutex. Always `static_assert(std::atomic<T>::is_always_lock_free)` on a hot
type.

`compare_exchange_weak` may fail spuriously (LL/SC architectures like ARM) so it
belongs in a loop; `_strong` retries internally. In a loop, use `weak`.

## 4. The memory model — the part they actually probe

The problem: both the **compiler** and the **CPU** reorder memory operations. x86-64
is relatively strong (TSO: only store→load reordering is visible); **ARM/aarch64 is
weakly ordered**, so code that "works" on your laptop can break on the target. Since
the FSD chip is ARM-based, this is a live concern, not a curiosity.

```cpp
std::memory_order_relaxed   // atomicity only, NO ordering with other operations
std::memory_order_acquire   // a load: nothing after it can move before it
std::memory_order_release   // a store: nothing before it can move after it
std::memory_order_acq_rel   // for RMW operations: both
std::memory_order_seq_cst   // default: a single total order over all seq_cst ops
std::memory_order_consume   // deprecated in practice; treat as acquire
```

**The release/acquire pairing is the one thing to know.** If thread A writes data,
then does a **release** store to an atomic, and thread B does an **acquire** load of
that same atomic and sees A's value, then **everything A wrote before the release is
visible to B after the acquire**. That is a *happens-before* edge, and it is how you
publish data without a lock.

```cpp
// Publishing a buffer, correctly, with no mutex:
Frame           g_frame;              // plain data
std::atomic<bool> g_ready{false};

// producer
g_frame = build();                                      // (1) plain writes
g_ready.store(true, std::memory_order_release);         // (2) release: (1) is published

// consumer
if (g_ready.load(std::memory_order_acquire)) {          // (3) acquire
    use(g_frame);                                       // (4) guaranteed to see (1)
}
```
With `relaxed` on both, (4) may read a half-built frame — on ARM, in practice, not
just in theory.

`seq_cst` is the default and the only ordering that composes intuitively (it gives a
single global total order, which is what you need for things like Dekker's algorithm
and the IRIW litmus test). It costs a full barrier: `mfence`/`lock` on x86, `dmb
ish` on ARM. Use it unless you have measured and can prove the weaker one correct.

`memory_order_relaxed` legitimate uses: a statistics counter nobody reads until the
end, a reference count **increment** (the decrement needs `acq_rel` because the last
one must see all prior writes before running the destructor), and a flag whose only
requirement is eventual visibility.

`std::atomic_thread_fence(std::memory_order_acquire)` gives you a standalone barrier
when you cannot attach the ordering to an access.

### Why `volatile` is not this
`volatile` prevents the **compiler** from eliding or reordering accesses *to that
object*. It emits **no CPU fences**, gives **no atomicity**, and creates **no
happens-before edge** — so it does nothing about the hardware reordering above, and a
`volatile` access is still a data race in the C++ model. It is for MMIO registers
and `volatile sig_atomic_t` in signal handlers. Full stop.

## 5. Task-based parallelism

```cpp
auto fut = std::async(std::launch::async, []{ return compute(); });
int result = fut.get();                     // blocks; rethrows any exception

std::promise<int> p; auto f = p.get_future();
std::thread{[&p]{ p.set_value(42); }}.detach();

std::packaged_task<int()> task{compute};
std::shared_future<int> shared = f.share();  // many waiters
std::latch  l{4};  l.count_down(); l.wait(); // C++20, single use
std::barrier b{4, []{ phase_done(); }};      // C++20, reusable
std::counting_semaphore<8> sem{8}; sem.acquire(); sem.release();
```
Caveats: `std::async` without an explicit launch policy may run **lazily on
`get()`**; the returned future's destructor **blocks** until the task finishes (a
classic surprise). `std::async` is not a thread pool — it may create a thread per
call. For real work, write or use a thread pool.

C++17 parallel algorithms are the easy win when they apply:
```cpp
#include <execution>
std::sort(std::execution::par_unseq, v.begin(), v.end());
std::transform_reduce(std::execution::par, a.begin(), a.end(), b.begin(), 0.0,
                      std::plus<>{}, std::multiplies<>{});
```
(gcc needs TBB linked for the parallel policies.) Remember: floating-point
reductions become non-deterministic (section 01).

## 6. The performance facts to quote

| Operation | Rough cost |
| --- | --- |
| L1 hit | ~1 ns (4 cycles) |
| L2 hit | ~4 ns |
| L3 hit | ~15-40 ns |
| DRAM | ~80-100 ns |
| uncontended `mutex` lock/unlock | ~20 ns (a futex, no syscall) |
| contended mutex → syscall + context switch | ~1-10 µs |
| atomic RMW, line in local L1 | ~5-20 cycles |
| atomic RMW, line owned by another core | ~100-200 cycles |
| false sharing | can be a 10-100x throughput loss |
| thread creation | ~10-30 µs |
| `std::this_thread::yield` / context switch | ~1-5 µs |

**False sharing**: two threads writing different variables in the same 64-byte cache
line. Fix with `alignas(std::hardware_destructive_interference_size)`. This is the
concurrency performance question that gets asked most often.

---

## Traps checklist

1. A data race is **UB**, not a wrong value.
2. `std::thread` must be joined or detached before destruction, or `terminate`.
   Use `std::jthread`.
3. Always use the **predicate** form of `cv.wait` (spurious and lost wakeups).
4. Take multiple locks via `std::scoped_lock` or in one global order.
5. Never call a callback or virtual function while holding a lock.
6. `volatile` is not atomic and emits no fences — it is for MMIO only.
7. `relaxed` gives atomicity but **no ordering**; publishing data needs
   release/acquire.
8. `compare_exchange_weak` can fail spuriously — use it in a loop.
9. `std::atomic<BigStruct>` may silently use a mutex — assert
   `is_always_lock_free`.
10. `std::async`'s future destructor blocks; without a launch policy it may be lazy.
11. False sharing can cost 10-100x — pad hot per-thread counters to a cache line.
12. x86 is strongly ordered; **ARM is not**. Test on the target.
