# 10 — Mock interview questions

## A. Rapid fire

1. Difference between undefined, unspecified, implementation-defined, and IFNDR.
2. Give six examples of undefined behavior.
3. Why is "it works in debug builds" not evidence that code is correct?
4. Which sanitizer finds what? Which two cannot be combined?
5. What is the runtime cost of an exception on the **non-throwing** path?
6. So what *do* exceptions cost? Give three costs.
7. Why do hard-real-time and embedded builds use `-fno-exceptions`?
8. When do you use `assert` vs. a return code vs. `optional` vs. `expected` vs. throw?
9. What happens if a `noexcept` function throws?
10. Which functions should always be `noexcept`, and why is one of them mandatory?
11. Why is `noexcept` part of your API contract?
12. Why must you never throw from a destructor?
13. In what order must `catch` clauses appear? Throw by value or pointer? Catch by what?
14. `sizeof(std::unique_ptr<T>)`? With a stateless custom deleter? Why?
15. `sizeof(std::shared_ptr<T>)`? What is in the control block?
16. What exactly is thread-safe about `shared_ptr` and what is not?
17. What is the cost of copying a `shared_ptr`, and why does it get worse with more threads?
18. `make_shared` vs `shared_ptr<T>(new T)` — two differences, and one downside of `make_shared`.
19. What is a `shared_ptr` cycle and how do you break it?
20. What does `enable_shared_from_this` solve? What happens if you misuse it?
21. What does a raw `T*` mean in a modern C++ interface?
22. State your ownership decision order in four steps.

## B. Find the bug

**B1.**
```cpp
int* get() { return new int(5); }
void use() { std::cout << *get(); }
```

**B2.**
```cpp
void process(std::shared_ptr<Frame> f);      // called at 1 kHz from 8 threads
```

**B3.**
```cpp
class Node {
    std::shared_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;
};
```

**B4.**
```cpp
void f() {
    auto p = new Widget;
    risky();                 // may throw
    delete p;
}
```

**B5.**
```cpp
try { ... }
catch (std::exception e)         { log(e.what()); }
catch (const SensorError& e)     { retry(); }
```

**B6.**
```cpp
class Connection {
    ~Connection() { if (!flush()) throw std::runtime_error("flush failed"); }
};
```

**B7.**
```cpp
std::vector<std::unique_ptr<Shape>> shapes;
auto c = std::make_unique<Circle>();
shapes.push_back(c);
```

**B8.**
```cpp
void on_data(std::function<void()> cb);
class Reader {
    std::vector<int> buf_;
public:
    void start() { on_data([this] { parse(buf_); }); }
};
```

**B9.**
```cpp
std::shared_ptr<Widget> global;
void worker() { global = std::make_shared<Widget>(); }     // 4 threads
```

**B10.**
```cpp
auto big = std::make_shared<std::array<std::byte, 64*1024*1024>>();
std::weak_ptr<std::array<std::byte, 64*1024*1024>> observer = big;
big.reset();
// how much memory is freed?
```

## C. Whiteboard

**C1.** Design the error-handling policy for `autonomy_core`, a foundations library
used both by a 100 Hz real-time planner and by an offline evaluation pipeline.
Specify what each layer uses and why.

**C2.** Implement `intrusive_ptr<T>` with a non-atomic refcount, and say when you
would prefer it to `shared_ptr` in this codebase.

**C3.** A profile shows 12% of frame time inside
`std::__shared_count::~__shared_count`. Diagnose and give three fixes.

**C4.** Write a function that acquires three resources (a file, a lock, and a DMA
buffer), any of which can fail, with no leaks on any path and no `goto`. Then do it
again under `-fno-exceptions`.

---
---

# Answers

**A1.** **UB**: no requirements whatsoever; the compiler may assume it never happens
and optimize on that basis. **Unspecified**: the implementation picks among several
valid behaviours and need not document the choice. **Implementation-defined**:
unspecified, but the implementation **must** document it. **IFNDR**: the program is
invalid, but no diagnostic is required — the classic case being an ODR violation.

**A2.** Signed integer overflow; out-of-bounds access; use-after-free /
use-after-return; dereferencing null; a data race; reading an uninitialized
(indeterminate) value; strict-aliasing violation; `INT_MIN / -1`; shifting by >= the
bit width; an infinite loop with no side effects; modifying a `const` object;
returning from a non-`void` function without a value; calling a pure virtual from a
constructor; reading an inactive union member.

**A3.** Because at `-O0` the compiler emits code that mirrors your source, so UB
often happens to do the "obvious" thing. At `-O2` the optimizer **assumes UB cannot
occur** and rewrites accordingly — it may delete a null check that a prior
dereference "proved" redundant, unroll a loop whose overflow you relied on, or hoist
a load across a data race. Debug behaviour is therefore no evidence of correctness;
sanitizers and UBSan are.

**A4.** **ASan** — out-of-bounds (heap/stack/global), use-after-free,
use-after-return, double free, and (with LSan) leaks; ~2x slowdown. **UBSan** —
signed overflow, misaligned access, invalid casts, null dereference, invalid enum
values; cheap. **TSan** — data races and lock-order inversions; ~5-15x slowdown.
**MSan** — reads of uninitialized memory; requires the whole program (including
libstdc++/libc++) instrumented. **ASan and TSan cannot be combined** (both use
shadow memory and intercept the same allocator); ASan and UBSan combine fine.

**A5.** **Zero.** With the Itanium "zero-cost" EH model there is no runtime check,
no flag test, and no branch on the happy path; the information needed to unwind lives
in a separate `.gcc_except_table`/`.eh_frame` section that is only consulted when a
throw actually occurs.

**A6.** (1) **Binary size** — unwind tables and RTTI typically add 5-15%, which
matters on a flash-constrained target. (2) **Unbounded, non-deterministic latency
when thrown** — the unwinder walks frames, matches types via RTTI (which involves
string comparison of type names across DSOs), and may take a global lock; ~1-10 µs,
with no WCET bound. (3) **Lost optimizations** — a call that can throw is a barrier:
the compiler must keep objects destructible at that point and cannot always reorder
or keep values in registers across it. Plus `throw` itself allocates.

**A7.** Because the throw path has **no bounded worst-case execution time** and, on
some implementations, takes a global lock and allocates — both disqualifying for a
hard-real-time deadline. Also because the unwind tables cost flash and some certified
toolchains (MISRA/AUTOSAR profiles) forbid them outright. The trade: you must then
return errors explicitly, which is what `std::expected` is for.

**A8.** **`assert`/contract** for *programmer errors* — violated preconditions that
indicate a bug (null argument, index out of range, invariant broken). These are not
recoverable and should abort in test builds. **Return code** for a C ABI boundary or
a `-fno-exceptions` build. **`std::optional`** when "absent" is a normal outcome
needing no explanation (a cache miss). **`std::expected<T,E>`** when the failure is
expected, recoverable, and the *reason* matters, especially in a hot path.
**Exceptions** for genuinely exceptional conditions, for constructor failure (the
only way to fail a constructor), and when the error must cross many frames that
cannot meaningfully handle it.

**A9.** `std::terminate` is called (via `std::unexpected`'s successor path). Stack
unwinding is **not guaranteed** to have happened, so destructors may not run. It is a
hard promise, not a hint.

**A10.** Move constructor, move assignment, `swap`, destructors (implicit), and
trivial observers like `size()`/`empty()`/`data()`. The **move operations are
effectively mandatory**: `std::vector` calls `std::move_if_noexcept` on
reallocation, so a throwing move forces it to **copy** every element to preserve the
strong exception guarantee.

**A11.** Because callers (and the standard library) branch on it: `noexcept(f())`
is observable, containers change algorithms based on it, and
`std::is_nothrow_move_constructible_v` gates optimizations. Adding `noexcept` later
is a compatible relaxation; **removing** it breaks callers that relied on it,
including `static_assert`s and container performance.

**A12.** Destructors run during stack unwinding. If a destructor throws while another
exception is propagating, you have two active exceptions and the runtime calls
`std::terminate`. Also destructors are implicitly `noexcept`, so a throw terminates
even outside unwinding. If a destructor's work can fail, expose an explicit
`close()`/`flush()` that returns a status and have the destructor swallow and log.

**A13.** **Most-derived first** — `catch` clauses are tried in order, and the first
one whose type matches wins, so a `catch (const std::exception&)` placed before
`catch (const SensorError&)` swallows everything. **Throw by value**, **catch by
`const&`** (catching by value slices derived exceptions and copies; catching by
pointer leaks). `catch (...)` last; re-raise with a bare `throw;` to preserve the
original exception.

**A14.** `sizeof(std::unique_ptr<T>)` == `sizeof(T*)` — one pointer. With a
**stateless** custom deleter it is still one pointer, because `unique_ptr` stores the
deleter as a (possibly empty) base/member subject to the **empty base
optimization**. A **stateful** deleter (a lambda with captures, a function pointer)
adds its size: `unique_ptr<FILE, int(*)(FILE*)>` is two pointers.

**A15.** Two pointers — one to the object, one to the control block. The control
block holds the **strong reference count**, the **weak reference count**, the
deleter, and the allocator (and, for `make_shared`, the object storage itself).

**A16.** Thread-safe: the **control block**. Multiple threads may copy, assign to
*their own* `shared_ptr` instances, and destroy `shared_ptr`s pointing at the same
object concurrently — the counts are atomic. **Not** thread-safe: the **pointee**
(you still need your own synchronization), and concurrent **writes to the same
`shared_ptr` object** — for that you need `std::atomic<std::shared_ptr<T>>` (C++20)
or the deprecated `std::atomic_store(&sp, ...)` free functions.

**A17.** An atomic read-modify-write (`lock xadd`) on copy and another on
destruction, plus a branch on the decrement. On a single core that is ~5-20 cycles;
across cores it is worse than that because the control-block cache line must be
acquired exclusively by each writing core, so it **ping-pongs**, and throughput
degrades roughly linearly with contending threads. Fix: pass `const shared_ptr&`
(no refcount change) or a plain `T&`/`T*` to functions that only observe, and only
copy when you genuinely extend lifetime.

**A18.** (1) `make_shared` performs **one** allocation for the object and control
block together; `shared_ptr<T>(new T)` performs **two**. (2) `make_shared` is
exception-safe in a multi-argument expression and mentions `T` once. (3) It can use
the allocator and gives better locality (object adjacent to the counts).
**Downside**: because the object and control block share one allocation, the
object's **memory is not released until the last `weak_ptr` dies** — so a large
object with long-lived weak observers keeps its full footprint alive. Also you cannot
use a custom deleter with `make_shared`.

**A19.** Two or more objects holding `shared_ptr`s to each other: each keeps the
other's strong count above zero, so neither is ever destroyed and both leak (LSan
reports it). Break it by making the **back-reference** a `std::weak_ptr` — the
owner-to-owned direction is `shared_ptr`, the owned-to-owner direction is `weak_ptr`,
locked when needed.

**A20.** It lets a member function hand out a `shared_ptr` to `*this` that shares the
**existing** control block — essential for async callbacks that must keep the object
alive (`[self = shared_from_this()]`). Misuse: calling it when the object is not
owned by a `shared_ptr` (a stack object, or inside the constructor) throws
`std::bad_weak_ptr`. Also, constructing a *new* `shared_ptr<T>(this)` instead creates
a second, independent control block → double delete.

**A21.** **Non-owning observation, possibly null.** It says "I look at this, I do not
manage its lifetime, and it may be absent". If it must exist, take a reference; if it
is a range, take `std::span`; if it is text, `std::string_view`; if you take
ownership, take a `unique_ptr` (or a value) so the signature says so.

**A22.** (1) A **value member or a local** — no indirection at all; this is the
default and covers most cases. (2) **`std::unique_ptr`** — exactly one owner, needed
for polymorphism, a stable address, or an incomplete type. (3) **`std::shared_ptr`**
— genuinely shared or indeterminate lifetime, especially async callbacks.
(4) **Raw pointer / reference / `span` / `string_view`** — observation only, never
ownership.

---

**B1.** Leaks on every call — the `new`ed `int` is never freed, and there is no name
to free it by. Return by value, or `std::unique_ptr<int>`, or just don't allocate.

**B2.** Taking `shared_ptr` **by value** means an atomic increment and decrement on
every one of those 8000 calls per second, on a control-block cache line shared by all
8 threads → coherence ping-pong. If the function only reads the frame, take
`const Frame&` (or `std::span`); if it might extend lifetime, take
`const std::shared_ptr<Frame>&` and copy only on the branch that needs to.

**B3.** A parent-child cycle: `children` holds strong references down, `parent` holds
strong references up, so no node's count ever reaches zero and the whole tree leaks.
Make `parent` a `std::weak_ptr<Node>`.

**B4.** If `risky()` throws, `delete p` never runs → leak. Also a plain leak if a
future `return` is added between the two lines. Use
`auto p = std::make_unique<Widget>();`, and the destructor handles every exit path.

**B5.** Two bugs: (a) `catch (std::exception e)` catches **by value**, which slices
the derived exception (losing `SensorError`'s data and its `what()` override) and can
itself throw on copy; (b) the order is wrong — the base handler comes first, so the
`SensorError` clause is **unreachable** and `retry()` never runs (gcc warns with
`-Wexceptions`). Catch by `const&`, most-derived first.

**B6.** Throwing from a destructor: during unwinding this calls `std::terminate`, and
since destructors are implicitly `noexcept` it terminates even outside unwinding.
Expose `[[nodiscard]] bool close()` (or return `std::expected`) for callers who care,
and have the destructor call `flush()` inside a `try/catch(...)` that logs. Also the
destructor is `private`-by-default here (`class`), which prevents stack allocation.

**B7.** `std::unique_ptr` is not copyable, so `push_back(c)` does not compile. Write
`shapes.push_back(std::move(c));`. (The error message is long; recognizing it
instantly is the point.)

**B8.** `[this]` captures a raw pointer; if the `Reader` is destroyed before the
callback fires, `parse(buf_)` is a use-after-free. Fix: derive from
`std::enable_shared_from_this<Reader>` and capture `[self = shared_from_this()]`, or
capture a `weak_ptr` and `lock()` inside the callback, or guarantee in the destructor
that all outstanding callbacks are cancelled/joined (and document it).

**B9.** Four threads **writing the same `shared_ptr` object** is a data race — the
atomic counts protect the control block, not the `shared_ptr`'s own two pointers, so
you can get a torn or double-decremented state. TSan flags it. Fix:
`std::atomic<std::shared_ptr<Widget>>` (C++20), or a mutex, or restructure so each
thread owns its own copy.

**B10.** **None of the 64 MB.** `make_shared` allocates the object and the control
block in a single block, and the block is only released when **both** the strong and
weak counts reach zero — so the live `weak_ptr` keeps all 64 MB resident even though
the object has been destroyed. Fix for large objects with long-lived observers:
`std::shared_ptr<T>(new T)` (two allocations, object freed at strong-count zero), or
do not hold a `weak_ptr` to something that big.

---

**C1.**
```
                     ┌─────────────────────────────────────────┐
  real-time layer    │ 100 Hz planner, perception, control      │
  (-fno-exceptions)  │  - preconditions: assert / contracts     │
                     │  - recoverable: std::expected<T,E> or a  │
                     │    status enum; NEVER throws, NEVER      │
                     │    allocates, bounded WCET               │
                     ├─────────────────────────────────────────┤
  foundations API    │ autonomy_core                            │
                     │  - every fallible function returns        │
                     │    [[nodiscard]] std::expected<T,Error>  │
                     │  - noexcept on everything that can be    │
                     │  - construction that can fail -> a static │
                     │    factory returning expected, not a     │
                     │    throwing constructor                  │
                     ├─────────────────────────────────────────┤
  offline / tooling  │ eval pipeline, log replay, CLI            │
  (exceptions on)    │  - exceptions fine: one try/catch at main │
                     │  - throws derived from std::exception     │
                     └─────────────────────────────────────────┘
```
Specifics to say out loud:
- The **library must not choose exceptions** for its core API, because a
  `-fno-exceptions` consumer cannot use it at all. So the API is
  `std::expected`-based, and the offline layer wraps it in a thin
  `value_or_throw()` helper — you can always add exceptions on top of expected, never
  the reverse.
- **Factories instead of throwing constructors** in the real-time layer, because a
  constructor's only failure channel is an exception:
  `static std::expected<Sensor, Error> Sensor::open(Config)`.
- **`assert`/`[[assume]]`/contracts for programmer errors**, compiled to a hard
  `std::abort` in test and CI builds (never silently disabled — use a custom
  `CHECK` that stays on in release for safety-relevant invariants, and say why:
  a corrupted invariant in a vehicle is worse than a controlled stop).
- **One error type**, an `enum class Error : std::uint8_t` plus an optional
  `std::error_code` bridge, so errors are cheap to return in a register and
  comparable without allocation. No `std::string` in an error type on the hot path.
- `[[nodiscard]]` on every fallible function, and a CI build with
  `-Werror=unused-result`.
- Say the trade-off honestly: `expected` puts a branch on the happy path that
  exceptions would not, and it clutters call sites — mitigated by `and_then`/
  `transform` and by keeping error propagation shallow.

**C2.**
```cpp
template <typename T>
class intrusive_ptr {
    T* p_{nullptr};
public:
    intrusive_ptr() noexcept = default;
    explicit intrusive_ptr(T* p) noexcept : p_{p} { if (p_) p_->add_ref(); }
    intrusive_ptr(const intrusive_ptr& o) noexcept : p_{o.p_} { if (p_) p_->add_ref(); }
    intrusive_ptr(intrusive_ptr&& o) noexcept : p_{std::exchange(o.p_, nullptr)} {}
    ~intrusive_ptr() { if (p_ && p_->release() == 0) delete p_; }

    intrusive_ptr& operator=(intrusive_ptr o) noexcept { std::swap(p_, o.p_); return *this; }
    T* get() const noexcept { return p_; }
    T& operator*()  const noexcept { return *p_; }
    T* operator->() const noexcept { return p_; }
    explicit operator bool() const noexcept { return p_ != nullptr; }
};

// The count lives IN the object -- hence "intrusive".
class RefCounted {
    mutable std::uint32_t refs_{0};        // non-atomic: single-threaded ownership
public:
    void add_ref() const noexcept { ++refs_; }
    std::uint32_t release() const noexcept { return --refs_; }
protected:
    ~RefCounted() = default;
};
```
When to prefer it here:
- **One pointer instead of two** (`sizeof(shared_ptr)` is 16, this is 8), so an array
  of them halves the memory traffic and fits twice as many per cache line.
- **No separate control block** → one allocation, and the count is in the *same* cache
  line as the object you are about to touch, so the refcount update is free rather
  than a second cache miss.
- **Non-atomic increments** when ownership never crosses a thread — a `lock xadd` is
  ~20 cycles and contends; `++` is one cycle. This is the actual win in a
  single-threaded 1 kHz loop.
- You can reconstruct a smart pointer from a raw `T*` safely (which
  `shared_ptr` cannot do without `enable_shared_from_this`), which matters at C API
  boundaries.

Costs to state: the count is non-atomic, so **it is UB if any object is shared across
threads** — that must be enforced by design and documented; `T` must derive from
your base, so it does not work for third-party or fundamental types; there is no
`weak_ptr` equivalent unless you build one; and it is easy to leak by mismatching
`add_ref`. Conclusion: use it only in a measured hot path, keep `shared_ptr` as the
default, and mention `boost::intrusive_ptr` as the existing implementation.

**C3.** Diagnosis: `~__shared_count` is the **atomic decrement plus the branch** on
the control block. 12% of frame time there means `shared_ptr`s are being copied and
destroyed at very high frequency, almost certainly because they are passed **by
value** through a call chain, or stored in a container that is being copied, and
several threads are touching the same control-block cache line so the `lock`
instruction is stalling on coherence traffic. Confirm with
`perf record -e cache-misses,mem_load_l3_miss_retired.remote_dram` and
`perf annotate` on the symbol, plus a count of `use_count()` churn.

Three fixes:
1. **Stop copying**: pass `const std::shared_ptr<T>&`, or better, `const T&` /
   `std::span` to everything that only observes. Usually removes most of the 12%
   for the cost of a signature change.
2. **Change the ownership model**: a single owner (`unique_ptr` plus non-owning
   references) if the lifetime is actually structured, or an **object pool / arena**
   with index handles so the "pointer" is a 4-byte index with no refcount at all.
   This is the right answer for a fixed-rate pipeline.
3. **Reduce contention**: if sharing is genuine, give each thread its own
   `shared_ptr` copy held for the whole frame (one inc/dec per frame instead of per
   call), or switch to a non-atomic `intrusive_ptr` if ownership never leaves a
   thread, or `std::atomic<std::shared_ptr<T>>` only where publication actually
   happens. Also consider hazard pointers / RCU (`folly::hazptr`) for a read-mostly
   structure.
Then re-measure and pin it: a benchmark in CI that fails if `use_count()` churn per
frame regresses.

**C4.**
```cpp
// With exceptions: RAII makes every path correct with no cleanup code at all.
Result open_all(const char* path) {
    FileDescriptor fd{path, O_RDONLY};        // throws on failure
    std::scoped_lock lock{dma_mutex_};        // releases on any exit
    DmaBuffer buf{kBufferBytes};              // throws on failure; frees in ~DmaBuffer
    return process(fd.get(), buf.span());     // if this throws, all three unwind
}                                             // no goto, no try, no leak

// Without exceptions (-fno-exceptions): factories return expected, RAII still does
// the releasing. The only change is that failure is a value, not a throw.
std::expected<Result, Error> open_all(const char* path) noexcept {
    auto fd = FileDescriptor::open(path, O_RDONLY);       // expected<FileDescriptor,Error>
    if (!fd) return std::unexpected(fd.error());

    std::scoped_lock lock{dma_mutex_};                     // RAII: unchanged

    auto buf = DmaBuffer::allocate(kBufferBytes);          // expected<DmaBuffer,Error>
    if (!buf) return std::unexpected(buf.error());         // fd + lock released here

    return process(fd->get(), buf->span());
}
```
The point to make explicitly: **RAII is what makes both versions leak-free** — the
error-handling mechanism only decides *how failure is reported*, not how resources
are released. That is why `goto cleanup` is a C idiom and has no place in C++. Add:
with `-fno-exceptions` the constructors must not be able to fail, hence the static
`open`/`allocate` factories; and if you want the monadic version,
`FileDescriptor::open(path, O_RDONLY).and_then([&](auto fd) { ... })` composes
without the `if (!x) return` ladder.
