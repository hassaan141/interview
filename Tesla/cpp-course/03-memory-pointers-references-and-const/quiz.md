# 03 — Mock interview questions

This is the section you will actually be interviewed on. Do it twice.

## A. Rapid fire

1. Five differences between a pointer and a reference.
2. What is `sizeof(r)` where `int& r = x;`?
3. Can a reference be null? What happens if you make one?
4. Difference between a wild, a dangling, and a null pointer.
5. Why is `f(nullptr)` better than `f(NULL)`?
6. What does `p + 1` advance by? When is pointer arithmetic UB?
7. Is it legal to *form* a pointer one past the end of an array? Two past?
8. Give the cost, in instructions, of a stack allocation vs. a heap allocation.
9. What is the default thread stack size on Linux, and what happens when you exceed it?
10. Is `delete nullptr` safe? Is `delete[]` on a `new` pointer safe?
11. What does `new (std::nothrow)` change?
12. What is placement new for and what must you remember to do?
13. `int x;` at block scope — what is its value? At file scope?
14. `std::vector<int> v{3,0};` vs `v(3,0);` — how many elements each?
15. What is the most vexing parse?
16. Why does braced initialization reject `int x{3.5}`?
17. Read out loud: `const int* p`, `int* const p`, `const int* const p`.
18. Difference between `const` and `constexpr` on a variable? On a function?
19. What does `consteval` add over `constexpr`? What does `constinit` fix?
20. What does `if constexpr` do that a runtime `if` cannot?
21. What is `volatile` for, and why is it not a threading tool?
22. List all five casts and one legitimate use of each.
23. When is `const_cast` + write UB?
24. What is strict aliasing? Name two legal ways to type-pun.
25. What does `dynamic_cast` require and what does it cost?
26. Why is `sizeof(BadLayout) == 24` but `sizeof(GoodLayout) == 16`?
27. Why is `sizeof(EmptyClass) == 1` but an empty base can be 0 bytes?
28. What is false sharing and which C++17 constant helps you avoid it?
29. What does `sizeof(arr)` give inside `void f(int arr[10])`?
30. What is the alignment of a struct, and why must `sizeof` be a multiple of it?

## B. Find the bug

**B1.**
```cpp
const std::string& name() {
    std::string s = build();
    return s;
}
```

**B2.**
```cpp
std::vector<int> v{1,2,3};
int* p = &v[0];
v.push_back(4);
std::cout << *p;
```

**B3.**
```cpp
struct Node { int v; Node* next; };
Node* head = new Node{1, new Node{2, nullptr}};
delete head;
```

**B4.**
```cpp
int* make() { int x = 5; return &x; }
```

**B5.**
```cpp
char buf[8];
auto* w = reinterpret_cast<std::uint64_t*>(buf);
*w = 0;
```

**B6.**
```cpp
void process(const std::vector<int>& data) {
    auto& mutable_ref = const_cast<std::vector<int>&>(data);
    mutable_ref.push_back(1);
}
const std::vector<int> input{1,2,3};
process(input);
```

**B7.**
```cpp
class Buffer {
    std::size_t n_;
    int* data_;
public:
    Buffer(std::size_t n) : n_{n}, data_{new int[n]} {}
    ~Buffer() { delete data_; }
};
```

**B8.**
```cpp
void log(const std::string& msg);
log("hello");                       // any cost concern?
std::string_view sv = std::string("temp");
std::cout << sv;
```

**B9.**
```cpp
struct Config { int timeout_ms; bool verbose; };
Config c;
if (c.verbose) { /* ... */ }
```

**B10.**
```cpp
constexpr int size = 1024 * 1024;
void f() { int big[size]; }
```

## C. Whiteboard

**C1.** Implement a minimal `unique_ptr` (ctor, dtor, move ctor, move assign,
`release`, `reset`, `get`, `operator*`, `operator bool`, deleted copies). Then
explain how `std::make_unique` differs from `unique_ptr<T>(new T)`.

**C2.** Implement a fixed-capacity `StaticVector<T, N>` that never allocates:
aligned storage, placement new, manual destruction, `push_back`, `emplace_back`,
`pop_back`, `~StaticVector`. Explain why an autonomy hot loop wants this.

**C3.** You are handed a struct that goes into a 1 MB ring buffer at 200 Hz.
Reduce its memory footprint and explain the cache consequences.
```cpp
struct Detection {
    bool valid;            // 1
    double confidence;     // 8
    std::uint8_t class_id; // 1
    double x, y, z;        // 24
    bool tracked;          // 1
    std::uint32_t frame;   // 4
};
```

**C4.** Explain exactly what happens, step by step, when this line executes and
`T`'s constructor throws: `auto p = std::make_unique<T>(args...);`

**C5.** A colleague writes `volatile bool stop_flag_;` to stop a worker thread.
Explain why it is wrong, what can go wrong concretely, and what to write instead.

---
---

# Answers

**A1.** A reference must be initialized; cannot be reseated; cannot legally be
null; has no arithmetic; `sizeof` gives the referent's size not a pointer size;
you cannot have an array of references; a reference is not itself an object (it
may not occupy storage at all).

**A2.** `sizeof(int)` — 4. `sizeof` on a reference reports the referred-to type.

**A3.** Not legally. `int& r = *static_cast<int*>(nullptr);` is UB at the point of
the dereference. In practice it often "works" until the optimizer, having assumed
`&r != nullptr`, deletes your null check.

**A4.** Wild = never initialized, points at garbage. Dangling = pointed-to object's
lifetime has ended (freed, scope exit, container reallocation) → use-after-free.
Null = `nullptr`, a well-defined "points at nothing"; only dereferencing is UB.

**A5.** `NULL` is `0` (or `0L`), an integer, so it participates in integer
overload resolution and can be ambiguous. `nullptr` has type `std::nullptr_t`
which converts to any pointer type and to nothing else.

**A6.** By `sizeof(*p)` bytes. It is UB if the result leaves the range
`[array, array + n]` (one past the end is allowed to form but not dereference),
or if `p` does not point into an array object at all, or on `void*`/function
pointers.

**A7.** One past the end: **yes**, legal to form and compare, illegal to
dereference. Two past: **UB to form**, even if you never read it.

**A8.** Stack: typically **zero** marginal instructions — the frame is one
`sub rsp, N` for the whole function, so an individual object is free. Heap:
tens to hundreds of instructions for a free-list/bin search, possibly a lock
(glibc arenas reduce but do not eliminate this), possibly an `mmap`/`brk`
syscall, plus a likely cache miss on the chunk header, plus fragmentation.
Order of magnitude: ~1 ns vs. ~50-200 ns.

**A9.** Main thread: 8 MB by default on Linux (`ulimit -s`). `pthread`/
`std::thread` default is usually 8 MB on glibc but commonly configured to
512 KB-2 MB in embedded builds. Exceeding it hits a guard page → `SIGSEGV`
with no diagnosable error; there is no `std::bad_alloc` for the stack. That is
why deep recursion and big stack arrays are banned in this kind of codebase.

**A10.** `delete nullptr` is a no-op, guaranteed safe. `delete[]` on a
`new T` pointer (or `delete` on a `new T[]` pointer) is **UB** — the array form
stores an element count the scalar form does not know about.

**A11.** It returns `nullptr` on failure instead of throwing `std::bad_alloc`.
Relevant for freestanding/embedded builds compiled with `-fno-exceptions`.

**A12.** Constructing an object in storage you already own, with no allocation —
the basis of object pools, fixed-capacity containers, `std::optional`,
`std::variant`, and small-buffer optimization. You must (1) ensure the storage is
correctly aligned (`alignas(T)`) and large enough, and (2) call the destructor
explicitly (`p->~T()`), because no `delete` will.

**A13.** At block scope with automatic storage duration: **indeterminate**;
reading it is UB (MSan/`-Wuninitialized` catch it). At file scope / `static` /
`thread_local`: **zero-initialized**, guaranteed.

**A14.** `{3,0}` → 2 elements `[3, 0]` (the `initializer_list` constructor wins).
`(3,0)` → 3 elements all `0`.

**A15.** A declaration that could be parsed as either an object definition or a
function declaration is parsed as a function declaration. `Widget w();` declares
a function; `Widget w(Foo());` declares a function taking a function pointer.
Fix with braces: `Widget w{};`.

**A16.** Because list-initialization forbids **narrowing conversions** — any
conversion that could lose value (float→int, wider int→narrower int, unless the
source is a constant expression whose value fits). It turns a silent truncation
into a compile error.

**A17.** `const int* p` = "pointer to a const int" (cannot write `*p`, can reseat
`p`). `int* const p` = "const pointer to int" (can write `*p`, cannot reseat).
`const int* const p` = both. Read right to left from the variable name.

**A18.** On a variable: `const` means "cannot be modified after initialization"
and may still be initialized at runtime; `constexpr` additionally requires
initialization by a constant expression and implies `const`. On a function:
`constexpr` means "usable in a constant expression *if* the arguments are
constant", it does not force compile-time evaluation; `const` on a member
function means "does not modify `*this`".

**A19.** `consteval` (an *immediate function*) **must** be evaluated at compile
time — calling it with a runtime value is a compile error. `constinit` asserts
that a static/thread-storage-duration variable is **constant-initialized**, so it
cannot participate in the static-initialization-order fiasco (no dynamic
initializer runs), while remaining mutable — unlike `constexpr`.

**A20.** The discarded branch is **not instantiated**, so it may contain code that
would be ill-formed for that `T` (`x.size()` on an `int`). A runtime `if` requires
both branches to compile. It also guarantees no branch at runtime.

**A21.** It tells the compiler that the value may change by means outside the
program, so each access must be emitted, in order, with no caching in a register
and no elision. Use: memory-mapped hardware registers, variables shared with a
signal handler (`volatile sig_atomic_t`), `setjmp` locals. It gives **no
atomicity** (a `volatile` 64-bit write may still be torn) and **no inter-thread
ordering** (it emits no fences and does not stop the *CPU* from reordering). Use
`std::atomic` for threads.

**A22.**
- `static_cast` — numeric conversion, `void*`→`T*`, unchecked downcast, invoking
  an explicit conversion operator, `std::move`-like reference casts.
- `const_cast` — calling a legacy C API that takes `char*` but does not modify.
- `reinterpret_cast` — turning an integer address from a device tree into a
  `volatile uint32_t*` for an MMIO register.
- `dynamic_cast` — a plugin/visitor boundary where you genuinely must query the
  dynamic type.
- `std::bit_cast` — reading the IEEE-754 bits of a float; hashing a POD.

**A23.** When the object it refers to was itself declared `const` (or is in
read-only memory). Casting away `const` to *read*, or to write to an object that
was never const, is fine; writing to an originally-`const` object is UB and may
fault in `.rodata`.

**A24.** The rule that an object may only be accessed through a glvalue of its
own type (or a cv-qualified / signed-unsigned variant, or `char`/`unsigned char`/
`std::byte`). The optimizer relies on it to keep values in registers across
stores through unrelated pointer types. Legal punning: `std::bit_cast` (C++20) or
`std::memcpy`; also a `union` in C but not C++; also viewing any object as
`std::byte`/`unsigned char` (one direction only).

**A25.** A polymorphic type (at least one virtual function) and RTTI enabled
(`-fno-rtti` disables it). Cost: a runtime walk of the type-info graph — tens of
nanoseconds, not constant time, and it defeats inlining. A `virtual` function or
a type tag/`std::variant` is almost always the better design.

**A26.** Alignment padding. `BadLayout{char, double, char}` needs the `double` at
an 8-byte boundary → 7 bytes after the first `char`, then 7 more at the end so
the struct size is a multiple of 8: 1+7+8+1+7 = 24. Reordering big→small gives
8+1+1+6 = 16. `-Wpadded` reports this.

**A27.** Two distinct objects must have distinct addresses, so a complete empty
object needs at least 1 byte. A base subobject does not have that requirement, so
the **empty base optimization** lets it take 0 bytes — which is how
`std::unique_ptr<T, StatelessDeleter>` is the size of one pointer. C++20
`[[no_unique_address]]` extends it to members.

**A28.** Two threads writing different variables that share a cache line: each
write invalidates the other core's line, so the line ping-pongs through the
coherence protocol and throughput collapses even with zero logical contention.
Fix by padding to
`std::hardware_destructive_interference_size` (C++17, usually 64). Its
counterpart `hardware_constructive_interference_size` is for data you *want*
together.

**A29.** `sizeof(int*)` — an array function parameter decays to a pointer; the
`[10]` is documentation only. Use `std::span<int, 10>`, `std::array<int,10>&`,
or a template parameter `template<std::size_t N> void f(int (&arr)[N])`.

**A30.** The struct's alignment is the maximum alignment of its members (or a
larger explicit `alignas`). `sizeof` must be a multiple of it so that
`arr[i]` in an array of the struct is still correctly aligned — hence trailing
padding.

---

**B1.** Returns a reference to a destroyed local → dangling, use-after-return
(`-Wreturn-local-addr` catches this one). Return by value: `std::string name()`
— NRVO makes it free.

**B2.** `push_back` may reallocate, invalidating `p`. Use-after-free. Any
reallocating operation invalidates all iterators, pointers, and references into a
`vector`. Re-acquire after, or `reserve()` up front, or use `std::deque`/`std::list`
whose element addresses are stable.

**B3.** Leaks the second node. `delete head` runs `~Node`, which does nothing
about `next`. Give `Node` ownership: `struct Node { int v; std::unique_ptr<Node>
next; };` — and then note that destroying a long list recurses, so a deep list
can blow the stack; iterative teardown is the fix.

**B4.** Returns the address of a local that has been destroyed → dangling
pointer. `-Wreturn-local-addr` catches it. Return by value or allocate.

**B5.** Two problems: alignment (`char buf[8]` is only 1-byte aligned, so the
`uint64_t` store may be misaligned → UB, and on some ARM configs a real fault),
and strict aliasing (accessing `char` storage through a `uint64_t` lvalue).
Fix: `alignas(std::uint64_t) std::byte buf[8];` and write with `std::memcpy`, or
declare the storage as a `std::uint64_t` in the first place.

**B6.** `input` is genuinely `const`, so writing through the `const_cast` is
**undefined behavior** — and `push_back` on a `const`-declared vector may fault
if it was placed in read-only memory. Also it is a lie in the interface: if the
function mutates, take a `std::vector<int>&`.

**B7.** Three bugs: (1) `new int[n]` paired with scalar `delete` → UB, must be
`delete[]`; (2) no copy constructor / copy assignment, so a copy double-frees
(rule of three/five violation); (3) `n == 0` allocates a zero-length array, which
is legal but easy to mishandle. Correct answer: `std::vector<int>` or
`std::unique_ptr<int[]>`, and then the rule of zero applies.

**B8.** `log("hello")` constructs a temporary `std::string` — a heap allocation
per call for anything over the SSO limit (~15 chars on libstdc++). Take
`std::string_view` if you only read it. The second line is worse: `sv` points into
a temporary `std::string` destroyed at the end of that statement, so printing it
is use-after-free. `string_view` never owns — that is the whole trap.

**B9.** `Config c;` default-initializes, leaving `timeout_ms` and `verbose`
**indeterminate**; reading `c.verbose` is UB (and a `bool` holding neither 0 nor 1
makes both branches of an `if` reachable in optimized code). Write `Config c{};`,
or give the members default member initializers.

**B10.** A 4 MB array on an 8 MB (or 512 KB thread) stack — stack overflow with
no diagnostic, just a SIGSEGV on the guard page. Heap-allocate it
(`std::vector<int>`), make it `static`, or use an arena.

---

**C1.**
```cpp
template <typename T>
class UniquePtr {
    T* p_{nullptr};
public:
    UniquePtr() noexcept = default;
    explicit UniquePtr(T* p) noexcept : p_{p} {}
    ~UniquePtr() { delete p_; }

    UniquePtr(const UniquePtr&)            = delete;   // unique = not copyable
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& o) noexcept : p_{o.release()} {}
    UniquePtr& operator=(UniquePtr&& o) noexcept {
        if (this != &o) { reset(o.release()); }        // self-assignment safe
        return *this;
    }

    T* release() noexcept { T* t = p_; p_ = nullptr; return t; }
    void reset(T* p = nullptr) noexcept { T* old = p_; p_ = p; delete old; }
    T* get() const noexcept { return p_; }
    T& operator*()  const noexcept { return *p_; }
    T* operator->() const noexcept { return p_; }
    explicit operator bool() const noexcept { return p_ != nullptr; }
};
```
Points to volunteer: `explicit` on the raw-pointer ctor (no accidental ownership
transfer) and on `operator bool` (so it does not silently convert to `int`);
`reset` deletes the *old* pointer **after** the assignment so self-reset is safe;
everything `noexcept`; the real `std::unique_ptr` takes a deleter as a template
parameter and uses EBO so a stateless deleter costs 0 bytes, and has an `T[]`
partial specialization that calls `delete[]`.

`make_unique<T>(args...)` vs. `unique_ptr<T>(new T(args...))`: single mention of
the type, no raw `new` in your code, guaranteed no leak if another argument in
the same call expression throws (the pre-C++17 `f(unique_ptr<A>(new A), g())`
leak), and it value-initializes. It cannot be used for custom deleters or to
adopt an existing pointer.

**C2.**
```cpp
template <typename T, std::size_t N>
class StaticVector {
    alignas(T) std::byte storage_[N * sizeof(T)];
    std::size_t size_{0};
    T* data() noexcept { return reinterpret_cast<T*>(storage_); }
public:
    StaticVector() = default;
    ~StaticVector() { clear(); }
    StaticVector(const StaticVector&) = delete;            // keep the demo short
    StaticVector& operator=(const StaticVector&) = delete;

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        assert(size_ < N && "StaticVector overflow");      // never grows
        T* slot = new (data() + size_) T(std::forward<Args>(args)...);
        ++size_;
        return *slot;
    }
    void push_back(const T& v) { emplace_back(v); }
    void pop_back() { assert(size_ > 0); data()[--size_].~T(); }
    void clear() { while (size_) pop_back(); }

    std::size_t size() const noexcept { return size_; }
    static constexpr std::size_t capacity() noexcept { return N; }
    T&       operator[](std::size_t i)       { return data()[i]; }
    const T& operator[](std::size_t i) const { return *(reinterpret_cast<const T*>(storage_) + i); }
    T* begin() noexcept { return data(); }
    T* end()   noexcept { return data() + size_; }
};
```
Why an autonomy hot loop wants it: **no allocation** → no `malloc` lock, no
page faults, no fragmentation, no unbounded worst-case latency (`malloc` has no
WCET bound); the storage is inline so it is contiguous and prefetch-friendly;
capacity is a compile-time constant so bounds and loop trip counts are known to
the optimizer. What to mention as caveats: `emplace_back` past `N` must be a hard
error (assert/`std::terminate`/an error return, never UB), the object is big so
don't pass it by value, and destructors must run exactly once — this is why the
real `std::inplace_vector` (C++26) exists.

**C3.**
```cpp
struct Detection {                 // 40 bytes, 8-byte aligned
    float x, y, z;                 // 12  — mm precision over 100 m needs ~17 bits;
    float confidence;              // 4     float has 24. double is waste here.
    std::uint32_t frame;           // 4
    std::uint16_t class_id;        // 2
    std::uint8_t  flags;           // 1   — valid:1, tracked:1, bits to spare
    std::uint8_t  reserved;        // 1   — explicit, so future fields are free
};
static_assert(sizeof(Detection) == 24 && alignof(Detection) == 4);
```
Original: `bool`(1) + 7pad + `double`(8) + `uint8`(1) + 7pad + 3×`double`(24) +
`bool`(1) + `uint32`(4) + 3pad = **56 bytes**, of which 17 are padding.
New: 24 bytes. Consequences to state:
- 56 → 24 bytes means 1 MB holds 18k instead of 7.8k detections, and a 64-byte
  cache line holds 2.7 detections instead of 1.1 — roughly **2.3x fewer cache
  misses and 2.3x less DRAM bandwidth** on a linear scan, which is what a
  bandwidth-bound loop actually costs.
- `double` → `float` halves the data *and* doubles SIMD lane count (8 floats vs.
  4 doubles per AVX2 register).
- Merging the two `bool`s into a flags byte removes a padding hole.
- Go further if the profile demands it: **SoA instead of AoS** —
  `struct Detections { std::vector<float> x, y, z; ... }` — so a pass that only
  reads `confidence` touches only that array. That is the answer that gets the
  follow-up conversation.
- Say explicitly: measure first (`perf stat -e cache-misses`), and pin the layout
  with `static_assert(sizeof(...))` so nobody regresses it.

**C4.** (1) `operator new(sizeof(T))` allocates raw storage — may throw
`std::bad_alloc`, in which case nothing else happens and nothing leaks.
(2) `T` is constructed in that storage via placement new with
`std::forward<Args>(args)...`. (3) If **that constructor throws**, the
placement-new expression is required to call the matching `operator delete` to
release the storage, the partially constructed object's already-constructed
members and bases are destroyed in reverse order, the exception propagates out of
`make_unique`, and `p` is never constructed — **no leak, no dangling**. (4) On
success the `unique_ptr` takes ownership; its destructor will `delete` the object
exactly once at scope exit, including during exception unwinding. The whole point
of `make_unique` is that step 3 is guaranteed.

**C5.** `volatile` gives you neither atomicity nor ordering. Concretely:
- Nothing stops the **CPU** from reordering the flag store after the stores to the
  data the worker is supposed to see, so the worker can observe `stop_flag_ ==
  true` and still read stale data (no release/acquire pairing, no fence emitted).
- On a non-lock-free width it can tear; `volatile` does not make an access
  indivisible.
- It is formally still a **data race** — UB — because `volatile` is not an atomic
  operation in the C++ memory model, so the standard gives you no guarantee at all,
  and tools like TSan will (correctly) report it.
- It also pessimizes: every read is re-fetched from memory, so it is *slower* than
  a relaxed atomic while being less correct.

Write:
```cpp
std::atomic<bool> stop_flag_{false};
// producer: stop_flag_.store(true, std::memory_order_release);
// worker:   while (!stop_flag_.load(std::memory_order_acquire)) { work(); }
```
`std::atomic<bool>` is lock-free everywhere that matters, the release/acquire pair
publishes everything written before the store, and if the worker *blocks* rather
than spins you want a `std::condition_variable` + `std::mutex` (or C++20
`atomic<bool>::wait`/`notify_all`) so you are not burning a core. Then add: the
*only* correct uses of `volatile` are MMIO registers and `volatile sig_atomic_t`
in signal handlers.
