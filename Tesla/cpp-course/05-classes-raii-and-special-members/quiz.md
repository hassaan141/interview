# 05 — Mock interview questions

## A. Rapid fire

1. Define RAII in one sentence. Why is it the answer to exception safety?
2. Name five resources other than memory that RAII manages.
3. What is a class invariant? Who establishes it, who preserves it?
4. What does `explicit` do on a one-argument constructor? On a conversion operator?
5. In what order are bases, members, and the constructor body run? What about destruction?
6. Are members initialized in init-list order or declaration order?
7. Why prefer the member-initializer list over assignment in the constructor body? Name two cases where it is mandatory.
8. What happens if you call a virtual function from a constructor?
9. List the six special member functions.
10. What does declaring a destructor do to the implicitly generated move operations?
11. What does declaring a move constructor do to the copy operations?
12. State the rule of zero, three, and five.
13. Difference between `= default` in the class body and out of line?
14. Why must a polymorphic base class have a `virtual` destructor?
15. Write copy assignment two ways. Why is copy-and-swap self-assignment safe for free?
16. What is `mutable` for, and give the one legitimate use?
17. Is `const` on a member function deep or shallow? Demonstrate.
18. What is a "hidden friend" and why is it preferred?
19. `static inline int x = 0;` in a class — what did C++17 change?
20. Can a constructor be `virtual`? Can a destructor? Why the asymmetry?
21. What is a delegating constructor, and what happens if the delegate throws?
22. Why should move operations be `noexcept`?

## B. Find the bug

**B1.**
```cpp
class Logger {
    std::ofstream file_;
    std::string   path_;
public:
    Logger(std::string path) : file_{path_}, path_{std::move(path)} {}
};
```

**B2.**
```cpp
class Shape { public: ~Shape() {} virtual double area() const = 0; };
std::unique_ptr<Shape> s = std::make_unique<Circle>();
```

**B3.**
```cpp
class Frame {
    std::vector<std::uint8_t> pixels_;
public:
    ~Frame() { /* nothing, just for symmetry */ }
};
std::vector<Frame> frames;
frames.push_back(make_frame());        // performance problem?
```

**B4.**
```cpp
class Handle {
    int fd_;
public:
    explicit Handle(const char* p) : fd_{::open(p, O_RDONLY)} {}
    ~Handle() { ::close(fd_); }
};
Handle a{"/dev/null"};
Handle b = a;
```

**B5.**
```cpp
class Widget {
public:
    Widget() { init(); }
    virtual void init() { /* base */ }
};
class Special : public Widget {
    std::vector<int> data_;
public:
    void init() override { data_.resize(10); }
};
```

**B6.**
```cpp
class Cache {
    std::map<int,int> m_;
public:
    int get(int k) const { return m_[k]; }
};
```

**B7.**
```cpp
class Matrix {
    double* d_;
public:
    Matrix(int n) : d_{new double[n * n]} {}
    ~Matrix() { delete d_; }
    Matrix& operator=(const Matrix& o) {
        delete[] d_;
        d_ = new double[o.n_ * o.n_];
        std::copy(o.d_, o.d_ + o.n_ * o.n_, d_);
        return *this;
    }
};
```

**B8.**
```cpp
class Timer {
    std::chrono::steady_clock::time_point start_;
public:
    Timer() : start_{std::chrono::steady_clock::now()} {}
    ~Timer() { report(elapsed()); }         // report() may throw
};
```

## C. Whiteboard

**C1.** Write a `FileDescriptor` RAII wrapper for a POSIX `int fd`: rule of five,
`noexcept` where correct, `release()`, `reset()`, `explicit operator bool`, and a
`swap`. Then explain what `std::unique_ptr` with a custom deleter would and would
not give you.

**C2.** Write a `LockGuard` and explain the difference between
`std::lock_guard`, `std::scoped_lock`, `std::unique_lock`, and
`std::shared_lock`, including when each is worth its cost.

**C3.** A colleague's `SensorBuffer` class shows up as 30% of frame time in a
profile, all of it in `memcpy`. The class is:
```cpp
class SensorBuffer {
    std::vector<float> data_;
public:
    SensorBuffer(const SensorBuffer&) = default;
    SensorBuffer& operator=(const SensorBuffer&) = default;
    ~SensorBuffer() { log_destruction(); }
};
```
Diagnose it and fix it.

---
---

# Answers

**A1.** Every resource is owned by an object that acquires it in its constructor
and releases it in its destructor, so the resource's lifetime is tied to a scope.
It is the answer to exception safety because destructors of fully constructed
objects are guaranteed to run during stack unwinding — so there is no path
(exception, early `return`, `break`, `goto`) on which the release is skipped, and
you never need `try/catch` just to clean up.

**A2.** File descriptors/handles, mutex locks, socket connections, GPU or DMA
buffers, memory mappings (`mmap`), thread joins (`std::jthread`), a
publisher/subscriber registration, a profiling/tracing span, a transaction, a
hardware interrupt mask, an OpenGL/CUDA context.

**A3.** A property that holds in every observable state of the object (e.g.
`size_ <= capacity_`, `kelvin_ >= 0`, "`p_` is either null or points to `n_`
valid elements"). The constructor establishes it (throwing if it cannot), every
public member function preserves it, and the destructor may assume it. `private`
data plus a narrow public interface is the enforcement mechanism.

**A4.** On a one-argument constructor it blocks implicit conversion and
copy-initialization (`Buffer b = 1024;` and `f(1024)` stop compiling). On a
conversion operator it blocks implicit use except in a boolean context
(`if (x)`, `!x`, `&&`, `||`, `?:`) — which is exactly why `explicit operator
bool()` gives you `if (ptr)` without `int n = ptr;`.

**A5.** Virtual bases (in order of the most-derived class), then direct
non-virtual bases in **declaration** order, then non-static data members in
**declaration** order, then the constructor body. Destruction is the exact
reverse: body, then members in reverse declaration order, then bases in reverse
order.

**A6.** **Declaration order**, always. The init-list order is ignored; writing it
in a different order gets you `-Wreorder` and, if one member's initializer reads
another, an uninitialized read.

**A7.** Assignment in the body first default-constructs (or value-initializes) the
member and then overwrites it — two operations instead of one, and it requires the
member to be default-constructible. Mandatory for: `const` members, reference
members, base-class subobjects, and any type with no default constructor.

**A8.** Dispatch resolves to the **current class's** override, not the derived
one — the derived subobject does not exist yet and its vptr has not been set. A
pure virtual call from a constructor is UB. Same in destructors (the derived part
is already gone). Fix: two-phase init, or a factory function that calls the
virtual after construction.

**A9.** Default constructor, copy constructor, copy assignment, move constructor,
move assignment, destructor.

**A10.** It **suppresses** them — no implicit move constructor or move assignment
is generated. The type still appears movable (`std::is_move_constructible_v` is
true) because the copy constructor binds an rvalue, so every "move" silently
performs a **copy**. That is a real performance bug and it is invisible to the
traits.

**A11.** It **deletes** the copy constructor and copy assignment.

**A12.** **Rule of zero:** own resources through RAII members and declare none of
the six — the compiler generates all of them correctly. **Rule of three
(pre-C++11):** if you need a destructor, copy constructor, or copy assignment, you
need all three. **Rule of five:** with move semantics, if you declare any of the
five, declare (or `= default`/`= delete`) all five.

**A13.** `= default` **in the class body** leaves the function trivial (if it
otherwise would be), so the type can remain trivially copyable / trivially
default-constructible — which is what lets `std::vector` relocate it with
`memmove` and lets you `memcpy` it into shared memory. Defaulting it **out of
line** makes it non-trivial (it is a user-provided function), which silently loses
those properties.

**A14.** Because `delete base_ptr` on a pointer whose static type is the base
and whose dynamic type is derived is **undefined behavior** unless the base's
destructor is virtual — in practice the derived members leak and the derived
destructor never runs. Alternatives: make the destructor `protected`
non-virtual (so you cannot delete through the base), or make the base
non-polymorphic.

**A15.** Copy-and-swap (`T& operator=(T other) { swap(*this, other); return
*this; }`) and the explicit form with a `this == &o` guard. Copy-and-swap is
self-assignment safe because the copy is made *into the parameter* before
anything is touched, and exception safe because if the copy throws, `*this` is
untouched. Its cost is one extra move for the rvalue case, which is why hot types
sometimes define both copy and move assignment separately.

**A16.** `mutable` lets a member be modified inside a `const` member function. The
legitimate uses are things not part of the object's observable value: a memoization
cache, a `std::mutex`, an atomic hit counter, a lazily computed derived value.
Anything else is a lie in the interface.

**A17.** **Shallow.** `const` applies to the member *objects*; a `T*` member
becomes `T* const`, not `const T* const`, so you can freely modify the pointee
from a `const` method. Demonstration: `void f() const { *owned_ = 99; }` compiles.
This is why `const` correctness does not survive raw pointers, and why
`std::unique_ptr<T>` behaves the same way (propagate-const was proposed and never
standardized).

**A18.** A `friend` function **defined inside** the class body. It is not a member,
it is not visible to ordinary unqualified lookup, and it can only be found by ADL
on its arguments. Benefits: it does not pollute the enclosing namespace, it cannot
be found by accident, both arguments get the same implicit-conversion treatment
(unlike a member `operator==`), and overload sets stay small, which speeds up
compilation and improves error messages. It is the modern way to write
`operator==`, `operator<<`, and `swap`.

**A19.** Before C++17, a `static` data member needed an out-of-line definition in
exactly one TU (`int Widget::count_ = 0;`) — a constant source of link errors.
C++17 `inline` variables let you define it in the class body, header-only, with
the linker merging the definitions.

**A20.** A destructor **can** be virtual and usually must be on a polymorphic
base. A constructor **cannot** — virtual dispatch requires an existing object
with an initialized vptr, and the constructor is what creates it. The pattern to
name instead is a **virtual clone** (`virtual std::unique_ptr<Base> clone() const`)
or a factory function.

**A21.** A constructor that calls another constructor of the same class in its
init-list (`Widget(int a) : Widget(a, 0) {}`), so validation lives in one place. If
the *delegate* completes and the delegating constructor's body then throws, the
object is considered constructed and **its destructor runs**. If the delegate
itself throws, no destructor runs (the object was never constructed).

**A22.** Because containers query it: `std::vector`'s reallocation uses
`std::move_if_noexcept`, so if your move constructor can throw, the vector
**copies** every element instead of moving them (to keep the strong exception
guarantee). A non-`noexcept` move is therefore a silent performance cliff. It also
enables better codegen and lets your type be used in `std::swap`-heavy algorithms
without pessimization.

---

**B1.** `file_` is declared before `path_`, so it is initialized **first**, from
`path_` — which is still empty. The file is opened with an empty name.
`-Wuninitialized`/`-Wreorder` may or may not catch it. Fix: declare `path_` first,
or initialize `file_` from the parameter directly.

**B2.** `Shape`'s destructor is non-virtual, so `delete` through
`std::unique_ptr<Shape>` is **undefined behavior** — `Circle`'s destructor never
runs and its members leak. Fix: `virtual ~Shape() = default;`. (And note that
declaring `~Shape(){}` also killed `Shape`'s implicit move operations.)

**B3.** The user-declared `~Frame()` suppresses the implicit move constructor, so
`push_back(make_frame())` **copies** the pixel buffer instead of stealing it — and
every `vector` reallocation copies every frame. Delete the empty destructor (rule
of zero) and the moves come back.

**B4.** `Handle b = a;` copies the `int fd_`, so both objects close the *same*
descriptor: double `close()`, and the second one may close a descriptor that has
since been reopened by another part of the program — a genuinely dangerous bug.
Also `open` failure (`fd_ == -1`) is never checked, and `close(-1)` is called.
Fix: delete the copies, add move operations that `std::exchange(fd_, -1)`, and
throw or store an error state on open failure.

**B5.** `Widget`'s constructor calls `init()`, which dispatches to
**`Widget::init`**, not `Special::init` — so `data_` is never resized. Worse, if
`Widget::init` were pure virtual it would be UB. Fix: do not call virtuals from
constructors; use a factory (`static std::unique_ptr<Special> create()`) that
constructs then calls `init()`, or pass the needed data into the constructor.

**B6.** `std::map::operator[]` is **non-const** — it inserts a default-constructed
value when the key is absent — so this does not compile in a `const` method. And
if you made `m_` `mutable` to "fix" it, `get` would silently mutate the map and
would not be thread-safe for concurrent readers. Use `m_.at(k)` (throws) or
`m_.find(k)` and handle the miss.

**B7.** Multiple defects: `new double[]` paired with scalar `delete` in the
destructor (UB — must be `delete[]`); no copy **constructor**, so a copy-constructed
`Matrix` shares the pointer and double-frees; `n_` is used but never declared or
initialized; copy assignment deletes the old buffer **before** allocating the new
one, so if `new` throws the object is left with a dangling pointer, and it is not
self-assignment safe (`m = m` frees its own data then reads it). Fix: hold a
`std::vector<double>` and apply the rule of zero, or use copy-and-swap.

**B8.** `report()` may throw from a destructor. If the `Timer` is destroyed during
stack unwinding from another exception, throwing calls `std::terminate`. Destructors
are implicitly `noexcept`, so the throw does not even propagate — it terminates.
Fix: wrap the body in `try { ... } catch (...) {}`, or make `report` `noexcept`
and have it log rather than throw.

---

**C1.**
```cpp
class FileDescriptor {
    int fd_{-1};
public:
    FileDescriptor() noexcept = default;
    explicit FileDescriptor(int fd) noexcept : fd_{fd} {}
    explicit FileDescriptor(const char* path, int flags) {
        fd_ = ::open(path, flags);
        if (fd_ < 0) throw std::system_error(errno, std::generic_category(), path);
    }
    ~FileDescriptor() { if (fd_ >= 0) ::close(fd_); }     // never throws

    FileDescriptor(const FileDescriptor&)            = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& o) noexcept : fd_{std::exchange(o.fd_, -1)} {}
    FileDescriptor& operator=(FileDescriptor&& o) noexcept {
        if (this != &o) { reset(o.release()); }
        return *this;
    }

    [[nodiscard]] int  get()     const noexcept { return fd_; }
    [[nodiscard]] int  release()       noexcept { return std::exchange(fd_, -1); }
    void reset(int fd = -1) noexcept {
        const int old = std::exchange(fd_, fd);
        if (old >= 0) ::close(old);
    }
    explicit operator bool() const noexcept { return fd_ >= 0; }
    friend void swap(FileDescriptor& a, FileDescriptor& b) noexcept {
        std::swap(a.fd_, b.fd_);
    }
};
```
Points to volunteer: `-1` (not 0) is the empty state, because 0 is a *valid* fd
(stdin) — a `unique_ptr`-style "null is empty" model is wrong here, which is the
main reason to hand-write this class. The destructor must not throw and must not
report `close` failures by throwing; if you care about `close` errors (you do, for
buffered writes on NFS) expose an explicit `close()` that returns a status and have
the destructor swallow. Everything is `noexcept` except the path constructor.
`[[nodiscard]]` on `release()` so you cannot leak by ignoring it.

`std::unique_ptr<int, Deleter>` **would** give you the rule-of-five boilerplate for
free; it **would not** give you the right empty value (it uses `nullptr`, and
`(void*)0` vs. fd 0 is a type-punning mess), it stores a pointer-sized member for
an `int`, and `explicit operator bool` would answer the wrong question. This is the
canonical example of "a handle is not a pointer".

**C2.**
```cpp
template <typename Mutex>
class LockGuard {
    Mutex& m_;
public:
    explicit LockGuard(Mutex& m) : m_{m} { m_.lock(); }
    ~LockGuard() { m_.unlock(); }
    LockGuard(const LockGuard&)            = delete;
    LockGuard& operator=(const LockGuard&) = delete;
};
```
- `std::lock_guard<M>` — the minimum: lock in the constructor, unlock in the
  destructor, not movable, no extra state. Zero overhead over a manual
  lock/unlock. Use it for a single mutex held for a whole scope.
- `std::scoped_lock<M...>` (C++17) — same, but variadic: it locks **multiple**
  mutexes with a deadlock-avoidance algorithm (`std::lock`). Prefer it over
  `lock_guard` as the default, and always over two nested `lock_guard`s.
- `std::unique_lock<M>` — movable, supports deferred locking
  (`std::defer_lock`), `try_lock`, timed locking, manual `unlock()`/`lock()`, and
  it is **required** by `std::condition_variable::wait`. Costs one extra `bool`
  and a branch in the destructor. Use it when you need any of that; not as a
  default.
- `std::shared_lock<M>` — acquires a `std::shared_mutex` in shared (reader) mode;
  many readers, one writer. Worth it only when reads greatly outnumber writes
  **and** the critical section is long enough to amortize the reader-count
  bookkeeping, which is itself a contended atomic — for short critical sections a
  plain mutex, or better, a lock-free/seqlock design, usually wins. Measure.

Then add the two points that matter for this role: hold locks for the shortest
possible time (never across I/O or a callback), and always take multiple locks in a
consistent global order (or via `scoped_lock`) to prevent deadlock.

**C3.** Diagnosis: `~SensorBuffer()` is user-declared, which **suppresses the
implicit move constructor and move assignment**. The explicitly defaulted *copy*
operations are still there, so every "move" — every `push_back` of a temporary,
every `std::vector` reallocation, every return by value that NRVO does not catch —
deep-copies the `std::vector<float>` via `memcpy`. That is precisely the 30%.

Fix, in order of preference:
```cpp
// (1) Rule of zero: delete the destructor entirely. If log_destruction() is
//     really needed, put it in a separate RAII member so SensorBuffer itself
//     declares nothing.
class SensorBuffer {
    std::vector<float> data_;
    DestructionLogger  log_;      // its own dtor does the logging
};

// (2) If the destructor must stay, restore the rule of five explicitly:
class SensorBuffer {
    std::vector<float> data_;
public:
    SensorBuffer() = default;
    SensorBuffer(const SensorBuffer&)                = default;
    SensorBuffer& operator=(const SensorBuffer&)     = default;
    SensorBuffer(SensorBuffer&&) noexcept            = default;
    SensorBuffer& operator=(SensorBuffer&&) noexcept = default;
    ~SensorBuffer() { log_destruction(); }
};
```
Verification, which is the part interviewers want to hear: add
`static_assert(std::is_nothrow_move_constructible_v<SensorBuffer>);` so the
regression cannot come back, then re-run the profile and confirm the `memcpy`
disappeared. Also mention `reserve()` on the containing vector to remove the
remaining reallocation copies, and that `noexcept` on the move is what lets
`std::vector` use `move_if_noexcept` instead of copying during growth.
