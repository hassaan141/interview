# 06 — Polymorphism, Virtual Dispatch, Operator Overloading, Object Layout

Course chapter: **10 (Object-Oriented Programming II)**

The performance half of this section (vtable cost, devirtualization, object
layout) is what a foundations team will actually push on.

---

## 1. The four kinds of polymorphism in C++

| Kind | Mechanism | Dispatch | Cost |
| --- | --- | --- | --- |
| **subtype / dynamic** | `virtual` + inheritance | runtime, via vtable | 1 indirect call, no inlining, 1-2 cache lines |
| **parametric** | templates | compile time | zero, fully inlinable, code bloat |
| **ad-hoc / overloading** | function overloads | compile time | zero |
| **coercion** | implicit conversions | compile time | conversion cost |
| (bonus) **static polymorphism** | CRTP | compile time | zero, see section 15 |
| (bonus) **type-erased value** | `std::variant` + `visit` | runtime, jump table | cheaper than virtual, no allocation |

The interview answer: "I reach for templates or `std::variant` when the set of
types is closed and known at compile time, and `virtual` when it is open — a
plugin boundary, or something that has to cross an ABI."

## 2. Virtual functions and the vtable

```cpp
struct Base {
    virtual ~Base() = default;
    virtual double area() const = 0;        // pure virtual -> abstract class
    virtual void draw() const { }           // has a default implementation
    void nonvirtual() { }
};
struct Circle final : Base {                // final: no further derivation
    double r{};
    double area() const override { return 3.14159 * r * r; }   // override: checked
    void draw() const final { }                                // final on a method
};
```

Layout of a polymorphic object (Itanium ABI — gcc/clang on Linux/ARM):

```
Circle object:            vtable for Circle:
+----------------+        +------------------------+
| vptr  (8 B)    | -----> | offset-to-top (0)      |
+----------------+        | typeinfo*  -> RTTI     |
| double r (8 B) |        | &Circle::~Circle       |  <- vptr points HERE
+----------------+        | &Circle::~Circle(del)  |
                          | &Circle::area          |
                          | &Circle::draw          |
                          +------------------------+
```

A virtual call is: load the vptr from the object (**possible cache miss**), load
the slot (**possible cache miss**), indirect call (**possible branch
mispredict**). Cost: ~1-3 ns warm, ~20-50 ns cold. The bigger cost is that
**the compiler cannot inline it**, which kills every downstream optimization —
that is the real number, not the indirect jump.

Facts worth stating:
- The vptr is added **once per class with virtual functions** (multiple
  inheritance gives you one vptr per polymorphic base), so `sizeof(Circle)` above
  is 16, not 8.
- The vptr is set by the constructor, base-first — hence "no virtual dispatch in
  a constructor".
- **Devirtualization**: if the compiler can prove the dynamic type (a local
  object, a `final` class or method, LTO + whole-program visibility,
  `-fwhole-program`, or speculative devirtualization with a guard) it inlines.
  `final` on classes and methods is therefore a *performance* annotation, not just
  a design one. Say this.
- `-fno-rtti` removes `typeinfo` and `dynamic_cast`; `-fno-exceptions` removes
  unwind tables. Both are common on embedded targets.

### `override` and `final`
`override` makes the compiler verify that you are actually overriding — it catches
the classic silent bug where a signature differs by `const` or a parameter type
and you have quietly created a *new* function. **Use `override` on every
override, always.**

```cpp
struct B { virtual void f(int) const; };
struct D : B { void f(int);          };  // NOT an override (missing const) -- hides
struct E : B { void f(int) const override; };  // compiler-checked
```

Other classic errors: non-virtual destructor in a base (UB on `delete`), a
`virtual` function with a **default argument** (default args are resolved
statically, so the base's default is used with the derived's body — never do it),
covariant return types are allowed but only for pointers/references to related
types.

### Name hiding
Declaring *any* `f` in a derived class hides **all** base `f` overloads:
```cpp
struct B { void g(int); void g(double); };
struct D : B { void g(int); };
D d; d.g(1.0);             // calls D::g(int) after converting! B::g(double) is hidden
struct E : B { using B::g; void g(int); };   // `using` brings them back
```

## 3. Inheritance access and casting

```cpp
class D : public    B {};   // is-a. B's public stays public
class D : protected B {};   // B's public becomes protected
class D : private   B {};   // implemented-in-terms-of; default for `class`
struct D : B {};            // default for `struct` is public
```
Prefer **composition** over private inheritance; the one real reason for private
inheritance is EBO on a stateless policy or to override a protected virtual.

Casting in a hierarchy:
```cpp
Derived d; Base* b = &d;                       // implicit upcast: always safe
auto* p = static_cast<Derived*>(b);             // downcast, UNCHECKED
auto* q = dynamic_cast<Derived*>(b);            // downcast, checked; nullptr on fail
auto& r = dynamic_cast<Derived&>(*b);           // throws std::bad_cast on fail
const std::type_info& t = typeid(*b);           // RTTI; typeid(*b) is polymorphic
assert(t == typeid(Derived));
```
A chain of `dynamic_cast`s is a design smell — it means you wanted a virtual
function, a visitor, or a `std::variant`.

## 4. Operator overloading

Rules of thumb:
- Overload only when the meaning is **obvious and conventional**. `+` for
  concatenation is fine; `+` for "register a callback" is a crime.
- Prefer **non-member (hidden friend)** for symmetric binary operators, so
  implicit conversions apply to both sides.
- Member for anything that mutates `*this`: `=`, `+=`, `[]`, `()`, `++`, `--`,
  `->`.
- `=`, `[]`, `()`, `->` **must** be members. `new`/`delete` are static members.
- Cannot overload: `.` `.*` `::` `?:` `sizeof` `alignof` `typeid` and the
  co_await/co_yield keywords' semantics.

```cpp
class Vec3 {
    double x_{}, y_{}, z_{};
public:
    constexpr Vec3() = default;
    constexpr Vec3(double x, double y, double z) : x_{x}, y_{y}, z_{z} {}

    constexpr Vec3& operator+=(const Vec3& o) noexcept {          // member, mutates
        x_ += o.x_; y_ += o.y_; z_ += o.z_; return *this;
    }
    // hidden friend, defined in terms of +=: symmetric and only findable by ADL
    friend constexpr Vec3 operator+(Vec3 a, const Vec3& b) noexcept { return a += b; }
    //                              ^ by value: the copy IS the result

    friend constexpr bool operator==(const Vec3&, const Vec3&) = default;  // C++20
    friend constexpr auto operator<=>(const Vec3&, const Vec3&) = default;

    constexpr double  operator[](std::size_t i) const { return (&x_)[i]; }
    constexpr double& operator[](std::size_t i)       { return (&x_)[i]; }

    friend std::ostream& operator<<(std::ostream& os, const Vec3& v) {
        return os << '(' << v.x_ << ',' << v.y_ << ',' << v.z_ << ')';
    }
    explicit constexpr operator bool() const noexcept { return x_||y_||z_; }
};
constexpr Vec3 operator*(double s, const Vec3& v) noexcept;   // scalar on the left
```

Increment/decrement — the canonical shapes:
```cpp
It& operator++()    { advance(); return *this; }              // prefix: return ref
It  operator++(int) { It tmp = *this; advance(); return tmp; } // postfix: dummy int,
                                                              // returns the OLD value
```

C++20 gives you `==` and `<=>` defaulted, and **rewrites** expressions: from
`operator==` the compiler synthesizes `!=`; from `<=>` it synthesizes `<`, `>`,
`<=`, `>=`, and reversed argument orders. Defining those two members replaces six
hand-written operators.

Multidimensional subscript (C++23): `operator[](std::size_t r, std::size_t c)`.
Before that, the idiom was `operator()(r, c)` — which is why Eigen and BLAS
wrappers use parentheses.

## 5. Object layout categories

```cpp
struct Aggregate      { int a; double b; };                // no user ctor, public
struct Trivial        { int a; Trivial() = default; };      // trivially copyable+
                                                            // trivially default-ctor
struct StandardLayout { private: int a; int b; };           // one access group
struct Pod            { int a; double b; };                 // trivial + std layout
struct NotStdLayout   { public: int a; private: int b; };   // two access groups
struct NotTrivial     { virtual ~NotTrivial() = default; };  // has a vptr
```

Definitions to be able to recite:
- **Aggregate**: no user-provided/inherited/explicit constructors, no private or
  protected non-static data members, no virtual functions, no virtual bases.
  → brace-initializable member-wise.
- **Trivially copyable**: every copy/move ctor, copy/move assignment, and the
  destructor is trivial (compiler-generated and non-virtual), and at least one of
  them is not deleted. → `memcpy` is a valid copy.
- **Standard layout**: all non-static data members have the same access control,
  no virtual functions or virtual bases, at most one class in the hierarchy has
  non-static data members, no member of the same type as the first base.
  → C-compatible offsets, `offsetof` is defined.
- **POD** = trivial **and** standard layout. (The term is deprecated in favour of
  the two precise traits.)

Why you care, in one sentence each: trivially copyable → you can `memcpy` it into
shared memory / a DMA buffer / a lock-free ring, and `std::vector` can relocate it
with `memmove`. Standard layout → C, another compiler, or a Python `ctypes`/`numpy`
view can agree on the offsets.

### Padding, EBO, `[[no_unique_address]]`
```cpp
struct Empty {};
static_assert(sizeof(Empty) == 1);                // distinct addresses
struct WithBase : Empty { int x; };
static_assert(sizeof(WithBase) == 4);             // empty base optimization
struct WithMember { Empty e; int x; };
static_assert(sizeof(WithMember) == 8);           // member takes a byte + padding
struct WithNUA { [[no_unique_address]] Empty e; int x; };
static_assert(sizeof(WithNUA) == 4);              // C++20 fixes it
```
EBO is how `std::unique_ptr<T, StatelessDeleter>` is one pointer wide, and how
`std::vector` with a stateless allocator costs three pointers and not four.

---

## Traps checklist

1. Non-virtual destructor in a polymorphic base → UB on `delete`.
2. No virtual dispatch inside constructors/destructors.
3. `override` on every override; without it a signature typo silently creates a
   new function.
4. Never give a virtual function a default argument.
5. Declaring any `f` in a derived class hides all base `f` overloads — `using B::f;`.
6. `final` is a performance tool (it enables devirtualization), not just a design one.
7. A virtual call cannot be inlined — that, not the indirect jump, is the cost.
8. Prefer hidden-friend non-member symmetric operators so conversions apply to
   both sides.
9. `operator=`, `[]`, `()`, `->` must be members.
10. C++20: define `==` and `<=>` and get the other six for free.
11. Mixed access control breaks standard layout; a vptr breaks trivial copyability.
