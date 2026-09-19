# 05 — Classes, RAII, Constructors/Destructors, the Special Member Functions

Course chapter: **9 (Object-Oriented Programming I)**

RAII is *the* C++ idea. If you can explain RAII and the rule of five precisely,
you sound like someone who writes C++ for a living.

---

## 1. RAII — Resource Acquisition Is Initialization

**Every resource is owned by an object whose constructor acquires it and whose
destructor releases it.** Because destructors run deterministically at scope exit
— including during exception unwinding, including on early `return`, including on
`break` — the resource cannot leak.

```cpp
class File {
    std::FILE* f_;
public:
    explicit File(const char* path, const char* mode) : f_{std::fopen(path, mode)} {
        if (!f_) throw std::runtime_error("open failed");   // never a half-built object
    }
    ~File() { if (f_) std::fclose(f_); }                    // always runs

    File(const File&)            = delete;                  // a file handle is unique
    File& operator=(const File&) = delete;
    File(File&& o) noexcept : f_{std::exchange(o.f_, nullptr)} {}
    File& operator=(File&& o) noexcept {
        if (this != &o) { if (f_) std::fclose(f_); f_ = std::exchange(o.f_, nullptr); }
        return *this;
    }
    std::FILE* get() const noexcept { return f_; }
};
```

Resources this applies to: heap memory (`unique_ptr`, `vector`), file
descriptors, sockets, mutex locks (`lock_guard`), GPU/DMA buffers, DDS/ROS
subscriptions, `perf` counters, a timing span, a log scope. The autonomy-flavoured
answer: "a `Frame` that owns a slot in a ring buffer releases it in its destructor,
so a `return` in the middle of a perception stage cannot strand the slot."

**Key property: RAII is exception safety.** Without it you need `goto cleanup` or
`try/catch` at every level.

## 2. Class invariants and `explicit`

A **class invariant** is a property true of every observable state of the object.
The constructor establishes it, every public method preserves it, the destructor
assumes it. `private` data plus a narrow public interface is how you enforce it.

```cpp
class Temperature {
    double kelvin_;                            // invariant: kelvin_ >= 0
public:
    explicit Temperature(double k) : kelvin_{k} {
        if (k < 0) throw std::invalid_argument("negative absolute temperature");
    }
    double kelvin() const noexcept { return kelvin_; }
};
```

`explicit` on a **single-argument constructor** blocks implicit conversion:
```cpp
class Buffer { public: explicit Buffer(std::size_t n); };
void f(Buffer);
// f(1024);      // error, good — 1024 is not conceptually a Buffer
f(Buffer{1024}); // intentional
```
`explicit` also applies to multi-argument constructors (blocks copy-list-init
`Buffer b = {1,2};`) and to **conversion operators** (`explicit operator bool()`
is why `if (ptr)` works but `int x = ptr;` does not). Rule: **`explicit` by
default**; drop it only for genuine value-equivalents (`std::string` from
`const char*`).

## 3. Constructors: order of operations

```cpp
struct Base { Base() { /* 1 */ } };
struct Member { Member() { /* 2 */ } };
struct Derived : Base {
    Member m_;
    int n_;
    Derived() : m_{}, n_{3} { /* 3: body */ }
};
```
Order: **virtual bases → direct bases (declaration order) → non-static data
members (declaration order) → constructor body**. Destruction is exactly the
reverse.

**Members are initialized in declaration order, not in the order you write the
init-list.** Getting that wrong reads an uninitialized member:
```cpp
class Bad {
    int n_;
    std::vector<int> v_;
public:
    Bad(int n) : v_(n), n_{n} {}     // v_ is built FIRST (declaration order),
};                                    // but reads nothing here; if v_(n_) it
                                      // would read an uninitialized n_.
// -Wreorder warns about the mismatch. Always write the list in declaration order.
```

Prefer the **member initializer list** over assignment in the body: assignment
default-constructs and then overwrites; the init-list constructs once. For
`const` members, references, and types without a default constructor, the
init-list is the *only* option.

**Default member initializers** (C++11) are the cleanest default:
```cpp
class Config {
    int  timeout_ms_ = 100;               // used unless a ctor overrides it
    bool verbose_    = false;
public:
    Config() = default;
    explicit Config(int timeout) : timeout_ms_{timeout} {}   // verbose_ still false
};
```

**Delegating constructors**:
```cpp
class Widget {
    int a_, b_;
public:
    Widget(int a, int b) : a_{a}, b_{b} { validate(); }
    Widget(int a) : Widget(a, 0) {}        // delegate — do NOT duplicate logic
    Widget() : Widget(0, 0) {}
};
```
Note: an exception escaping a delegating constructor destroys the
already-delegated-to object correctly. And calling a virtual function from a
constructor dispatches to the **current class's** override, not the derived one —
the derived part does not exist yet. That is a classic question.

## 4. The special member functions (the "rule of five")

```cpp
class T {
    T();                          // default constructor
    T(const T&);                  // copy constructor
    T& operator=(const T&);       // copy assignment
    T(T&&) noexcept;              // move constructor
    T& operator=(T&&) noexcept;   // move assignment
    ~T();                         // destructor
};
```

### The generation rules (memorize this table)

| If you declare... | you suppress... |
| --- | --- |
| any constructor | the default constructor |
| a destructor | the **move** ctor and move assignment (copy ops are still generated, but **deprecated**) |
| a copy ctor or copy assign | the **move** ctor and move assignment (and deprecates the other copy op) |
| a move ctor or move assign | the **copy** ctor and copy assignment (they are **deleted**) |

The trap that follows: **declaring a destructor silently kills your move
operations**, so a class with a `~T()` copies where you expected it to move. That
is a real, measurable performance bug.

### Rule of zero / three / five
- **Rule of zero** (what you should do): own resources via RAII members
  (`vector`, `unique_ptr`, `string`), declare **none** of the six, and the compiler
  generates correct copy/move/destroy for free.
- **Rule of three** (pre-C++11): if you need any of destructor / copy ctor / copy
  assign, you need all three.
- **Rule of five**: with move semantics, if you need any of the five, declare all
  five (or `= default` / `= delete` them explicitly).

```cpp
// Rule of zero: nothing declared, everything correct.
class Frame {
    std::vector<std::uint8_t> pixels_;
    std::unique_ptr<Metadata> meta_;      // makes Frame move-only, automatically
};
```

### `= default` and `= delete`
```cpp
class Widget {
public:
    Widget() = default;                         // the trivial one, still trivial
    Widget(const Widget&) = delete;             // non-copyable
    Widget& operator=(const Widget&) = delete;
    Widget(Widget&&) noexcept = default;        // move-only
    Widget& operator=(Widget&&) noexcept = default;
    virtual ~Widget() = default;                // polymorphic base: virtual dtor!
};
```
`= default` **in the class body** keeps the function trivial (so the type can stay
trivially copyable); `= default` **out of line** does not. That matters for
`std::is_trivially_copyable` and therefore for `memcpy`-ability.

### Copy assignment: the two correct shapes
```cpp
// (a) copy-and-swap: exception safe and self-assignment safe for free
T& operator=(T other) {          // by value: the copy happens in the parameter
    swap(*this, other);
    return *this;
}
// (b) explicit, with a self-check
T& operator=(const T& o) {
    if (this == &o) return *this;
    // ... release old, acquire new; acquire BEFORE release for exception safety
    return *this;
}
```

## 5. Class keywords

```cpp
class Widget {
    static int count_;                 // one per class; needs an out-of-line def
                                       // (or `static inline int count_ = 0;` C++17)
    mutable std::mutex m_;             // modifiable even in a const method
    mutable std::optional<int> cache_; // the legitimate use: memoization
public:
    int  id() const noexcept;          // const method: *this is const
    void set(int);                     //   -> two const overloads are allowed
    Widget& self() { return *this; }   // `this` is a POINTER (T* const)

    using value_type = int;            // nested type alias, for generic code
    friend class Inspector;            // grants access; use sparingly
    friend std::ostream& operator<<(std::ostream&, const Widget&);   // the usual one
    void operator=(const Widget&) = delete;
    static constexpr int kMaxId = 1024;    // in-class initialized constant
};
int Widget::count_ = 0;                    // pre-C++17 out-of-line definition
```

`const` member functions: `this` is `const T*`, so you cannot modify members —
**except** `mutable` ones, and except through a pointer member (the pointer is
const, the pointee is not — "shallow const", a great interview question). A
`const` and a non-`const` overload of the same function is the standard
accessor pair; C++23 lets you deduplicate them with an explicit object parameter.

`static` member functions have no `this`, can be called as
`Widget::make()`, and can be used as plain function pointers.

`friend` breaks encapsulation deliberately; it is the right tool for
`operator<<`, for a hidden-friend `operator==`, and for a builder/test fixture.
Prefer **hidden friends** (defined inside the class) — they are only findable by
ADL, which keeps overload sets small and speeds up compilation.

---

## Traps checklist

1. Members are initialized in **declaration** order, regardless of the init-list
   order (`-Wreorder`).
2. Declaring a destructor **suppresses** the implicit move operations.
3. Declaring a move operation **deletes** the copy operations.
4. A polymorphic base needs a `virtual` (or `protected`) destructor.
5. Calling a virtual function from a constructor/destructor dispatches to the
   current class, not the derived one.
6. `explicit` by default on single-argument constructors and conversion operators.
7. `= default` inside the class keeps triviality; out of line does not.
8. Prefer the member init list over assignment in the body; it is mandatory for
   `const`, reference, and non-default-constructible members.
9. `const` on a method is shallow: `T* p_` stays writable through the pointer.
10. Rule of zero first; rule of five if you must; never rule of one.
