# 09 — Move Semantics, Value Categories, Forwarding References, Copy Elision

Course chapter: **21 (Advanced Topics I)**

Do this section early. Half of what the STL does only makes sense once you have
it, and "explain move semantics" is asked in essentially every C++ interview.

---

## 1. Value categories — the actual taxonomy

Every expression has a type **and** a value category:

```
            expression
           /          \
      glvalue          rvalue
     /       \        /      \
 lvalue    xvalue(both)    prvalue
```

- **lvalue** — has identity, cannot be moved from. A named variable, `*p`, `a[i]`,
  a function call returning `T&`.
- **prvalue** — no identity, can be moved from. A literal, `a + b`, a function call
  returning `T` by value, `T{}`.
- **xvalue** — has identity **and** can be moved from. `std::move(x)`,
  a function call returning `T&&`, `a[i]` on an rvalue array.
- **glvalue** = lvalue ∪ xvalue ("has identity").
- **rvalue** = prvalue ∪ xvalue ("can be moved from").

Mnemonic: **identity** (can I take its address / does it have a name?) and
**movability** (is it safe to steal its guts?). lvalue = identity, not movable.
prvalue = no identity, movable. xvalue = both.

Since C++17, a prvalue is *not* an object — it is an "initializer for an object",
and it only *materializes* into an object (an xvalue) when needed. That is what
makes guaranteed copy elision work.

```cpp
int   x = 1;
int&  lr = x;          // x is an lvalue
int&& rr = 5;          // 5 is a prvalue; rr is a named rvalue reference...
                       // ...and `rr` used as an expression is an LVALUE.
int&& rr2 = std::move(x);   // std::move(x) is an xvalue

// THE rule people get wrong: a named rvalue reference is an lvalue.
void sink(std::string&&);
void f(std::string&& s) {
    sink(s);              // ERROR: `s` is an lvalue here
    sink(std::move(s));   // correct
}
```

## 2. Rvalue references and move semantics

```cpp
void f(const T&);   // binds lvalues AND rvalues (fallback)
void f(T&);         // binds non-const lvalues only
void f(T&&);        // binds rvalues only (prvalues and xvalues)
```

A **move** is a transfer of ownership: the source's resources are given to the
destination, and the source is left in a **valid but unspecified** state.

```cpp
class Buffer {
    std::size_t n_{};
    int*        p_{};
public:
    Buffer(Buffer&& o) noexcept
        : n_{std::exchange(o.n_, 0)}          // leave the source EMPTY, not garbage
        , p_{std::exchange(o.p_, nullptr)} {}

    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) {                      // self-move must not destroy
            delete[] p_;
            n_ = std::exchange(o.n_, 0);
            p_ = std::exchange(o.p_, nullptr);
        }
        return *this;
    }
};
```

"Valid but unspecified" for the standard library means:
`std::string`/`std::vector` are left **empty** in every real implementation but the
standard only guarantees you may call any operation with no preconditions
(`size()`, `clear()`, assignment). A moved-from `std::unique_ptr` **is** guaranteed
null. Never *read* a moved-from object's value; you may reassign or destroy it.

`std::move` is **not** a move. It is a cast:
```cpp
template <typename T>
constexpr std::remove_reference_t<T>&& move(T&& t) noexcept {
    return static_cast<std::remove_reference_t<T>&&>(t);
}
```
It generates no instructions. It only changes which overload is selected.

Corollaries:
- `std::move` on a `const T` gives `const T&&`, which binds to `const T&` → you get
  a **copy**, silently. `const` kills moves.
- `std::move` on a trivially copyable type does nothing useful (the "move" is a copy
  either way).
- `return std::move(local);` **pessimizes** — it defeats NRVO. Just `return local;`.

## 3. Forwarding (universal) references and perfect forwarding

```cpp
template <typename T> void f(T&& x);   // FORWARDING reference: T is DEDUCED here
void g(Widget&& x);                     // plain rvalue reference: no deduction
template <typename T> void h(std::vector<T>&& x);   // NOT forwarding (not just T&&)
auto&& r = expr;                        // forwarding reference
```
A forwarding reference is exactly `T&&` where `T` is a template parameter of that
same function being deduced (or `auto&&`).

**Reference collapsing** (the rule that makes it work):
```
T&  &  -> T&      T&  && -> T&
T&& &  -> T&      T&& && -> T&&
```
"An lvalue reference anywhere wins." Combined with the special deduction rule —
if the argument is an lvalue of type `U`, `T` is deduced as `U&`; if an rvalue,
`T` is deduced as `U` — you get:

```cpp
Widget w;
f(w);              // T = Widget&,  parameter type Widget& &&  -> Widget&
f(Widget{});       // T = Widget,   parameter type Widget&&
f(std::move(w));   // T = Widget,   parameter type Widget&&
```

**Perfect forwarding** preserves both the type and the value category:
```cpp
template <typename... Args>
std::unique_ptr<T> make(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
template <typename T>
constexpr T&& forward(std::remove_reference_t<T>& t) noexcept {
    return static_cast<T&&>(t);           // T& && -> T&  (lvalue stays lvalue)
}                                          // T  && -> T&& (rvalue stays rvalue)
```
Rules: `std::forward<T>` **must** be given the template parameter explicitly, and
you use it **exactly once** per argument (forwarding twice may move twice). Use
`std::move` for a named rvalue reference parameter, `std::forward<T>` for a
forwarding reference.

Gotchas with forwarding-reference constructors:
```cpp
class Person {
    std::string name_;
public:
    template <typename S>
    explicit Person(S&& s) : name_{std::forward<S>(s)} {}   // greedy!
    Person(const Person&) = default;
};
Person p{"a"};
Person q{p};        // picks the TEMPLATE (exact match Person&) over the copy ctor!
```
Fix: constrain it —
`template <typename S> requires (!std::same_as<std::remove_cvref_t<S>, Person>)`.

## 4. Copy elision, RVO, NRVO

- **RVO** (returning an unnamed temporary) is **mandatory** since C++17: there is no
  copy or move, the object is constructed directly in the caller's storage.
  ```cpp
  Widget make() { return Widget{}; }        // guaranteed: zero constructions extra
  ```
- **NRVO** (returning a named local) is **allowed but not required** — every real
  compiler does it at `-O1`+, but it is not guaranteed and is disabled by
  `-fno-elide-constructors`.
  ```cpp
  Widget make() { Widget w; return w; }     // NRVO: usually elided
  Widget make2(bool b) { Widget a, c; return b ? a : c; }   // NRVO cannot apply
  ```
- If elision does not apply, the return is treated as an **rvalue**, so it moves,
  not copies (implicit move on return, extended in C++20/23).

Consequences worth stating:
```cpp
Widget make() { Widget w; return std::move(w); }   // BAD: disables NRVO
                                                   // (-Wpessimizing-move)
Widget f() { return std::move(global); }            // here std::move IS needed
void g(Widget w);  g(make());                       // zero copies, zero moves
```
Also: **return by value is not slow.** This is the single most useful fact to state
out loud, because it kills the "out-parameter for performance" habit.

## 5. Type deduction summary

| Form | Deduction | Keeps `const`? | Keeps refs? | Array decays? |
| --- | --- | --- | --- | --- |
| `template <T> f(T)` | by value | no | no | yes |
| `template <T> f(T&)` | by ref | yes | n/a | no |
| `template <T> f(const T&)` | by ref | (always const) | n/a | no |
| `template <T> f(T&&)` | forwarding | yes | yes | no |
| `auto x = e` | like `T` by value | no | no | yes |
| `auto& x = e` | like `T&` | yes | n/a | no |
| `auto&& x = e` | forwarding | yes | yes | no |
| `decltype(auto) x = e` | `decltype` rules | yes | yes | no |

`auto(x)` (C++23) is an explicit **decay-copy** — useful when you must break
aliasing:
```cpp
void erase_first(std::vector<int>& v) {
    std::erase(v, auto(v.front()));   // copy first! v.front() dangles mid-erase
}
```

## 6. Ref-qualifiers and `const` correctness

```cpp
class Builder {
    std::string s_;
public:
    const std::string& str() const&  { return s_; }            // on an lvalue: view
    std::string        str() &&      { return std::move(s_); } // on an rvalue: steal
    Builder& add(char c) &  { s_ += c; return *this; }          // lvalue only
    Builder  add(char c) && { s_ += c; return std::move(*this); } // rvalue chaining
};
auto s = Builder{}.add('a').add('b').str();   // moves out, no copies
```
Ref-qualifiers let a method behave differently depending on whether `*this` is an
lvalue or an rvalue — the standard trick for "steal the buffer if the object is
about to die". `std::optional::value() &&` and `std::move(opt).value()` work this
way.

**`const` correctness**: mark every member function that does not modify observable
state `const`; take `const&` for read-only parameters; make `const` the default and
mutability the exception. It is not only hygiene — `const` enables the compiler to
keep values in registers across calls and it is required for a type to be usable
from multiple reader threads without synchronization.

But remember (section 05): `const` is **shallow** — a `T*` member stays writable.

---

## Traps checklist

1. A **named** rvalue reference is an lvalue — you must `std::move` it again.
2. `std::move` is a cast; it generates no code and moves nothing by itself.
3. `std::move` on a `const` object silently copies.
4. `return std::move(local)` defeats NRVO — never write it.
5. A moved-from object is valid but **unspecified**; do not read its value.
6. Move operations must be `noexcept` or `std::vector` copies instead.
7. `T&&` is a forwarding reference **only** when `T` is being deduced right there.
8. `std::forward<T>` needs the explicit template argument, and use it once.
9. A forwarding-reference constructor out-competes the copy constructor — constrain it.
10. RVO is guaranteed (C++17); NRVO is not, but is universal in practice.
11. Return by value is not slow. Stop using out-parameters for speed.
