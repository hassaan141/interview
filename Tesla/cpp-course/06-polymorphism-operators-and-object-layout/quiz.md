# 06 — Mock interview questions

## A. Rapid fire

1. Name four kinds of polymorphism in C++ and their dispatch time.
2. Draw the memory layout of an object with one virtual function.
3. What exactly happens, in loads and jumps, on a virtual call?
4. What is the *dominant* cost of a virtual call, and why?
5. How much does a vptr add to `sizeof`? What about multiple inheritance?
6. When does the compiler devirtualize? Name three enablers.
7. Why is `final` a performance annotation?
8. What does `override` protect you from? Give the classic silent bug.
9. Why must a polymorphic base have a virtual destructor?
10. Why should a virtual function never have a default argument?
11. What is name hiding, and how do you undo it?
12. What does `typeid(*p)` give for a polymorphic `p`? For a non-polymorphic one?
13. Cost and requirements of `dynamic_cast`. What does a chain of them mean?
14. Which operators must be members? Which cannot be overloaded at all?
15. Why prefer a hidden-friend `operator+` over a member `operator+`?
16. Write prefix and postfix `++` for an iterator. Which is cheaper?
17. What do you get for free from defaulting `operator==` and `operator<=>`?
18. Define aggregate, trivially copyable, standard layout, POD.
19. What does trivially copyable buy you? Standard layout?
20. What is the empty base optimization, and what does `[[no_unique_address]]` add?
21. What is `-fno-rtti` and what breaks?
22. When would you choose `std::variant` + `visit` over `virtual`?

## B. Find the bug

**B1.**
```cpp
class Base { public: ~Base() { } virtual void run() = 0; };
void go(Base* b) { b->run(); delete b; }
```

**B2.**
```cpp
struct Filter { virtual void apply(Image& img, int radius = 3); };
struct Blur : Filter { void apply(Image& img, int radius = 5) override; };
Filter* f = new Blur; f->apply(img);        // which radius?
```

**B3.**
```cpp
struct Sensor { virtual void configure() { } Sensor() { configure(); } };
struct Lidar : Sensor { void configure() override { rings_ = 128; } int rings_{}; };
```

**B4.**
```cpp
struct Base { virtual int id() const { return 1; } };
struct Derived : Base { int id() { return 2; } };
Base* b = new Derived; b->id();
```

**B5.**
```cpp
class Money {
    long cents_;
public:
    Money(long c) : cents_{c} {}
    Money operator+(const Money& o) const { return Money{cents_ + o.cents_}; }
};
Money m{100};
auto total = 50 + m;         // compiles?
```

**B6.**
```cpp
struct Packet { std::uint32_t id; virtual ~Packet() = default; };
Packet p{42};
std::memcpy(shared_memory, &p, sizeof p);
```

**B7.**
```cpp
class Iter {
public:
    Iter operator++() { Iter old = *this; advance(); return old; }
};
```

**B8.**
```cpp
struct Visitor {
    void visit(Shape* s) {
        if (auto* c = dynamic_cast<Circle*>(s)) { /* ... */ }
        else if (auto* q = dynamic_cast<Square*>(s)) { /* ... */ }
        else if (auto* t = dynamic_cast<Triangle*>(s)) { /* ... */ }
    }
};
```

## C. Whiteboard

**C1.** A perception stage iterates 100k objects at 30 Hz calling one virtual
method each. Estimate the dispatch overhead, then give three ways to remove it
with the trade-offs of each.

**C2.** Design the interface for a pluggable `Detector` that autonomy application
teams implement. Decide virtual vs. template vs. variant, decide the ownership
model, and say how you keep the ABI stable across independently built plugins.

**C3.** Implement `Matrix<T>` with `operator()(row, col)`, `operator[]` (C++23
multi-arg), row-major storage, a `constexpr` `at()` with bounds checking, and
`operator*`. Explain the layout choice and what it means for cache behaviour.

---
---

# Answers

**A1.** Subtype/dynamic (`virtual` + inheritance, runtime), parametric (templates,
compile time), ad-hoc (function/operator overloading, compile time), coercion
(implicit conversions, compile time). Bonus: static polymorphism via CRTP
(compile time) and type-erased value semantics via `std::variant` + `visit`
(runtime, but a jump table rather than a pointer chase).

**A2.** `[vptr (8 bytes)][members...]`. The vptr points into a per-class vtable
containing (Itanium ABI) offset-to-top, a pointer to the `type_info`, then the
virtual function pointers in declaration order — the complete-object destructor and
the deleting destructor take two consecutive slots.

**A3.** (1) Load the vptr from the object's first 8 bytes; (2) load the function
pointer from `vptr[slot]`; (3) indirect `call`. Two dependent loads, each of which
can miss cache, followed by a jump the branch predictor must get right via the
indirect-target buffer.

**A4.** Not the indirect jump — it is that **the compiler cannot inline through
it**, so it cannot constant-fold, vectorize, hoist, or eliminate anything across
the call boundary, and it must assume the callee clobbers memory. The measured
cost of the jump itself is ~1-3 ns warm; the lost optimization is frequently
10-100x that in a tight loop.

**A5.** One pointer (8 bytes on LP64), once per class that introduces virtual
functions. With multiple (non-virtual) inheritance from *n* polymorphic bases you
get *n* vptrs. Virtual inheritance adds a vbase offset/VTT and makes member access
itself indirect.

**A6.** When it can prove the dynamic type: (1) the object is a local/by-value
object of known type; (2) the class or the method is `final`; (3) LTO plus
`-fvisibility=hidden`/`-fwhole-program` so the compiler knows no other TU derives
from it; (4) speculative devirtualization with a type guard from profile data
(PGO). Also `__attribute__((final))`-style annotations and sealing a hierarchy in
an anonymous namespace.

**A7.** Because `final` tells the compiler that no further override can exist, so a
call through a base pointer to a `final` method, or any call on a `final` class,
has exactly one possible target and can be inlined. Marking leaf classes `final`
is a free, measurable optimization.

**A8.** It asserts at compile time that the function really overrides a base
virtual. The classic silent bug is a signature mismatch —
`void f(int) const` in the base, `void f(int)` in the derived — which creates a new
non-overriding function that *hides* the base one, so calls through a base pointer
silently run the base implementation. Same for a wrong parameter type, a missing
`const`, or a misspelled name.

**A9.** Because `delete p` where `p` is `Base*` pointing at a `Derived` is
**undefined behavior** unless `~Base` is virtual — in practice only `~Base` runs,
so `Derived`'s members leak and its invariants are never torn down. Alternative:
make `~Base` `protected` and non-virtual so deletion through the base does not
compile.

**A10.** Default arguments are resolved from the **static** type at the call site,
while the function body is chosen by the **dynamic** type. So
`Filter* f = new Blur; f->apply(img);` runs `Blur::apply` with `Filter`'s default
`radius = 3`. That combination is always a bug. Use an overload that forwards, or a
`std::optional` parameter.

**A11.** Declaring a name in a derived class hides **every** base declaration of
that name, including all overloads and even different arities — the derived scope
is searched first and lookup stops there. Undo with `using Base::f;` in the derived
class.

**A12.** For a polymorphic type, `typeid(*p)` is evaluated at runtime and reports
the **dynamic** type (and `*p` *is* evaluated, so side effects happen). For a
non-polymorphic type it is a compile-time constant reporting the **static** type
and the operand is unevaluated.

**A13.** Requires a polymorphic type and RTTI enabled; cost is a call into
`__dynamic_cast`, which walks the type-info graph — not constant time, tens of
nanoseconds, and not inlinable. A chain of `dynamic_cast`s means the design is
missing something: add a virtual function, use the visitor pattern, or switch to a
closed set with `std::variant` + `std::visit`.

**A14.** Must be members: `operator=`, `operator[]`, `operator()`, `operator->`,
and the conversion operators (plus `operator->*`). Cannot be overloaded at all:
`.` `.*` `::` `?:` `sizeof` `alignof` `typeid`, and you cannot invent new
operators or change precedence/arity.

**A15.** A member `operator+` only allows implicit conversion on the **right**
operand — `m + 50` works, `50 + m` does not. A non-member allows conversions
symmetrically on both sides. Making it a *hidden friend* (defined in the class
body) additionally keeps it out of ordinary lookup, so it is only found by ADL on
its own arguments, which shrinks overload sets and speeds up compilation.

**A16.**
```cpp
It& operator++()    { ++p_; return *this; }               // prefix: no copy
It  operator++(int) { It old = *this; ++p_; return old; } // postfix: one copy
```
Prefix is cheaper (or equal); the `int` parameter is an unused tag that
distinguishes the two.

**A17.** From `operator==`: `!=`, plus reversed-argument forms. From
`operator<=>`: `<`, `>`, `<=`, `>=`, and their reversed forms. Defaulted, they
compare bases then members **in declaration order**, lexicographically. Two
declarations replace six hand-written operators, and they are `constexpr` and
`noexcept` when the members are.

**A18.**
- **Aggregate**: no user-provided/inherited/`explicit` constructors, no private or
  protected non-static data members, no virtual functions, no virtual bases →
  brace-initializable member-wise.
- **Trivially copyable**: all copy/move constructors and assignments and the
  destructor are trivial and non-deleted (at least one copy/move is not deleted)
  → `memcpy` is a valid copy.
- **Standard layout**: all non-static data members in one access-control group, no
  virtual functions or virtual bases, at most one class in the hierarchy has
  non-static data members, and the first member is not of the same type as a base
  → C-compatible offsets, `offsetof` well defined.
- **POD**: trivial + standard layout. Deprecated as a term; say the two traits.

**A19.** Trivially copyable → `memcpy`/DMA/shared memory/lock-free ring buffers are
valid, and `std::vector` can relocate with `memmove` instead of element-wise moves.
Standard layout → a C translation unit, a differently-compiled C++ TU, or a
`numpy`/`ctypes` view can agree on the byte offsets, and `offsetof` is defined.

**A20.** A base-class subobject is not required to have a unique address, so an
**empty base** can occupy zero bytes. This is how `std::unique_ptr<T, Stateless>`
is exactly one pointer wide. `[[no_unique_address]]` (C++20) extends the same
permission to **members**, so an empty allocator/comparator/policy member stops
costing a byte plus padding.

**A21.** A compiler flag that removes run-time type information: `typeid` on
polymorphic types and `dynamic_cast` stop working, and `type_info` objects are not
emitted. It shrinks binaries meaningfully and is common on embedded/freestanding
targets (together with `-fno-exceptions`). Virtual dispatch itself still works.

**A22.** When the set of types is **closed** and known at compile time. You get:
no heap allocation per object, contiguous storage (so a `std::vector<Variant>` is
cache-friendly), no vptr in each object, compile-time exhaustiveness if your
visitor has no generic fallback, and value semantics (copyable, comparable,
trivially relocatable when the alternatives are). Choose `virtual` when the set is
**open** — a plugin loaded at runtime, or a boundary that must stay ABI-stable.

---

**B1.** `~Base` is not virtual, so `delete b` through `Base*` is UB and the derived
destructor never runs. Also, declaring `~Base(){}` suppressed `Base`'s implicit
move operations. Fix: `virtual ~Base() = default;`.

**B2.** `Blur::apply` runs, with `radius = 3` — the base's default, because default
arguments are bound statically from `Filter*`. Never put a default argument on a
virtual function.

**B3.** `Sensor`'s constructor calls `configure()`, which dispatches to
**`Sensor::configure`** (the vptr still points at `Sensor`'s vtable, and `Lidar`'s
members do not exist yet), so `rings_` is never set — and had `configure` been
pure virtual, it would be UB. Fix: a factory that constructs then configures, or
pass the configuration into the constructor.

**B4.** `Derived::id()` is **not** an override — it is missing `const`, so it is a
new, non-virtual function that hides `Base::id`. `b->id()` calls
**`Base::id`** and returns 1. `override` would have made it a compile error.

**B5.** No. `operator+` is a member, so the left operand must already be a
`Money`; `50 + m` finds no viable operator. (Had it been a non-member, the
non-`explicit` constructor would convert `50` — which is its own design question:
make the constructor `explicit` and provide named factories, or accept the
conversion deliberately.) Fix: `friend Money operator+(Money, const Money&)`.

**B6.** `Packet` has a virtual destructor, so it has a **vptr** and is neither
trivially copyable nor standard layout. Copying its bytes into shared memory copies
a **pointer into this process's vtable**, which is meaningless (and a security
problem) in the reading process — and `memcpy`-ing over a polymorphic object is UB.
Fix: make the wire type a plain aggregate with no virtuals, `static_assert` it is
trivially copyable and standard layout, and keep the polymorphism in a separate
non-serialized class.

**B7.** Wrong signature: with no parameter this is **prefix** `++`, but it behaves
like postfix (returns the old value by copy). Every `++it` in every loop now makes
a copy and, worse, callers relying on `(++it)->x` get the pre-increment element.
Fix: `Iter& operator++()` returning `*this`, and a separate
`Iter operator++(int)`.

**B8.** A `dynamic_cast` chain: O(n) RTTI lookups per call, unextendable (every new
shape edits this function — an open/closed violation), and silently does nothing
for an unhandled type. Replace with a virtual method on `Shape`, a proper
double-dispatch visitor (`virtual void accept(Visitor&)`), or, if the set is
closed, `std::variant` + `std::visit`, which also gives you compile-time
exhaustiveness.

---

**C1.** 100k × 30 = 3M virtual calls/second. At ~2 ns of dispatch each (warm
i-cache, predictable target) that is ~6 ms/s = **0.6% of wall time** — so *lead
with*: the dispatch itself is almost certainly not your problem, and if the objects
are heap-allocated the **cache misses on the pointer chase** (~100 ns each, up to
300 ms/s = 30%) dominate the dispatch by two orders of magnitude. Measure with
`perf stat -e cache-misses,branch-misses` before changing anything.

Three ways to remove it:
1. **Sort/partition by type and call in batches** (`std::vector<Circle>`,
   `std::vector<Square>`). The call devirtualizes per batch, memory becomes
   contiguous, and the loop vectorizes. Costs: a partitioning pass and a less
   uniform API. Usually the biggest win by far, because it fixes the *memory*
   problem, not the dispatch.
2. **`std::variant` + `visit` over a contiguous `std::vector<Variant>`**. Removes
   the allocation and the pointer chase, keeps a single homogeneous container,
   trades the vtable for a jump table. Requires a closed type set.
3. **Templates/CRTP**, so dispatch happens at compile time. Zero overhead, full
   inlining and vectorization; costs code bloat, build time, and the type set must
   be known at the call site.
Also mention `final` on leaf classes (free devirtualization) and LTO
(cross-TU devirtualization) as the two changes that cost nothing.

**C2.**
```cpp
// Stable, virtual-based plugin boundary.
struct DetectorV1 {
    virtual ~DetectorV1() = default;
    // Only C-layout types cross the boundary: no std::string, no std::vector,
    // no exceptions -- those are not ABI-stable across compilers/std versions.
    [[nodiscard]] virtual int detect(const FrameView& in, DetectionSpan out,
                                    std::size_t* out_count) noexcept = 0;
    [[nodiscard]] virtual std::uint32_t abi_version() const noexcept = 0;
};
extern "C" DetectorV1* tesla_create_detector_v1();   // C linkage: no name mangling
extern "C" void        tesla_destroy_detector_v1(DetectorV1*);
```
Decisions and why:
- **virtual**, because the set of detectors is *open* — application teams add new
  ones, possibly in separately compiled shared objects. Templates would require
  recompiling the framework; `std::variant` would require the framework to know
  every type.
- **Ownership**: the plugin allocates and the plugin frees (paired
  `create`/`destroy` `extern "C"` functions), because allocators may differ across
  DSOs. Hand the caller a `std::unique_ptr<DetectorV1, DestroyFn>` on the
  framework side so RAII still applies.
- **ABI stability**: freeze the vtable — never reorder, remove, or insert virtual
  functions in a published interface; add a `DetectorV2` that inherits or a new
  factory symbol instead. Pass only trivially copyable, fixed-width, standard-layout
  types across the boundary (spans of PODs, not `std::vector`/`std::string`).
  `noexcept` on every boundary function, because exceptions do not reliably
  propagate across DSOs built with different toolchains. Version explicitly
  (`abi_version()` plus a versioned factory symbol name). Compile with
  `-fvisibility=hidden` and export only the factory.
- For the **internal** pipeline, use templates or `variant` — reserve virtual for
  the actual plugin seam. Say that out loud: "virtual at the boundary, static
  inside."

**C3.**
```cpp
template <typename T, std::size_t Rows, std::size_t Cols>
class Matrix {
    std::array<T, Rows * Cols> d_{};                      // row-major, contiguous
public:
    static constexpr std::size_t rows = Rows, cols = Cols;

    constexpr T&       operator()(std::size_t r, std::size_t c)       { return d_[r * Cols + c]; }
    constexpr const T& operator()(std::size_t r, std::size_t c) const { return d_[r * Cols + c]; }
#if __cpp_multidimensional_subscript >= 202110L            // C++23
    constexpr T&       operator[](std::size_t r, std::size_t c)       { return (*this)(r, c); }
    constexpr const T& operator[](std::size_t r, std::size_t c) const { return (*this)(r, c); }
#endif
    constexpr T& at(std::size_t r, std::size_t c) {
        if (r >= Rows || c >= Cols) throw std::out_of_range("Matrix::at");
        return (*this)(r, c);
    }
    constexpr std::span<T, Cols> row(std::size_t r) { return {d_.data() + r * Cols, Cols}; }

    template <std::size_t N>
    constexpr Matrix<T, Rows, N> operator*(const Matrix<T, Cols, N>& rhs) const {
        Matrix<T, Rows, N> out{};
        for (std::size_t i = 0; i < Rows; ++i)             // i-k-j order: the inner
            for (std::size_t k = 0; k < Cols; ++k) {       // loop walks rhs's row
                const T a = (*this)(i, k);                 // contiguously
                for (std::size_t j = 0; j < N; ++j)
                    out(i, j) += a * rhs(k, j);
            }
        return out;
    }
};
static_assert(sizeof(Matrix<float, 3, 3>) == 36);          // no padding, no pointer
```
Layout justification to say out loud:
- **Row-major, single contiguous array** (not `array<array<T,C>,R>` of pointers):
  one allocation (here, none — it is inline), no pointer chase, and `row(i)` is a
  contiguous span the hardware prefetcher and the vectorizer both understand.
- **Dimensions as template parameters** make `sizeof` known, put small matrices on
  the stack, let the compiler unroll and fully vectorize, and make
  dimension mismatches a **compile error** rather than a runtime check — the right
  trade for a 3×3 or 4×4 transform in a robotics stack. For runtime-sized matrices
  you would take dimensions as constructor arguments and store a `std::vector`, or
  use `std::mdspan` over caller-owned memory.
- **Loop order matters more than anything else here**: the naive `i-j-k` order
  strides through `rhs` by `N` elements, missing cache on every access; `i-k-j`
  keeps both `rhs` and `out` walking contiguously, which is typically 2-5x faster
  before any tiling. For anything large, say the real answer: **tile it, or call
  into BLAS/Eigen** — do not hand-write GEMM in production.
- `operator()` for the general case because `operator[]` only took one argument
  before C++23; provide both, guarded by the feature-test macro.
