# 04 — Functions, Overloading, Lambdas, the Preprocessor

Course chapter: **8 (Basic Concepts VI)**

---

## 1. Parameter passing — the decision table

| Want | Signature | Notes |
| --- | --- | --- |
| read a small scalar / trivially copyable ≤ 2 words | `void f(T v)` | by value; copying is cheaper than an indirection |
| read a large object | `void f(const T& v)` | no copy, no ownership |
| read a string you don't own | `void f(std::string_view s)` | avoids constructing a `std::string` per call |
| read a contiguous range | `void f(std::span<const T> s)` | replaces `(ptr, len)`; keeps `size()` |
| modify the caller's object | `void f(T& v)` | in/out parameter |
| take ownership | `void f(T v)` then `std::move` inside, or `void f(std::unique_ptr<T>)` | sink parameter; `unique_ptr` makes the transfer explicit in the signature |
| forward to something else | `template<class U> void f(U&& v)` | forwarding reference + `std::forward` |
| optional argument | `void f(std::optional<T>)` or an overload | avoid `T*` meaning "maybe" |

`std::span` and `std::string_view` are **non-owning views** — never store one
whose backing store may die first. That is the single most common modern-C++ bug.

Pass-by-pointer only when the argument is genuinely optional, when you are
talking to C, or when you must rebind. Otherwise a reference documents "must
exist".

## 2. Overload resolution, in the order the compiler applies it

1. **Name lookup** builds the candidate set (including ADL candidates).
2. Remove non-**viable** candidates (wrong arity, no conversion exists,
   constraints not satisfied).
3. Rank the viable ones by the *worst* argument conversion:
   exact match > promotion > standard conversion > user-defined conversion >
   ellipsis. Non-template beats template on a tie. More-specialized template
   beats less-specialized.
4. Ambiguity → compile error. A deleted function winning → compile error.

A function's **signature** for overloading is: name, parameter types (after
top-level-cv and reference-to-pointer decay for by-value), cv/ref qualifiers on a
member function, and template parameters. The **return type is not part of it**:

```cpp
int  f(int);
// double f(int);   // ERROR: cannot overload on return type alone
void g(int);
void g(const int);  // SAME function — top-level const on a by-value param is ignored
void h(int*);
void h(const int*); // different: the const is not top-level
```

`= delete` is an overload-resolution tool, not just "forbidden":
```cpp
void set_timeout(std::chrono::milliseconds);
void set_timeout(int) = delete;              // reject raw ints at compile time
void set_timeout(double) = delete;
```

Default arguments: evaluated **at the call site**, may only be added in one
declaration (usually the header — never repeat it in the definition), and must be
trailing. They do **not** create an overload, so they can create ambiguity with
one.

## 3. Function pointers and function objects

```cpp
int add(int a, int b) { return a + b; }
int (*fp)(int, int) = &add;                // function pointer: one indirect call,
                                           // not inlinable through the pointer
using Fn = int (*)(int, int);

struct Adder {                             // function object / functor:
    int bias;                              // carries state
    int operator()(int a, int b) const { return a + b + bias; }
};                                         // inlinable — the type is known!

std::function<int(int,int)> f = add;       // type-erased: heap allocation
                                           // possible, virtual-like dispatch
```

**The performance point to make:** passing a lambda or functor as a *template
parameter* lets the compiler inline it (`std::sort` beats C `qsort` for exactly
this reason). `std::function` erases the type, so you get an indirect call and
possibly an allocation. In a hot path use a template parameter, or
`function_ref`/`std::function_ref` (C++26) for a non-owning, non-allocating
callable view.

## 4. Lambdas — the part that gets you

```cpp
auto l = [captures](params) specifiers -> ReturnType { body };
```

A lambda is sugar for a unique, unnamed **closure class** with an
`operator()` that is `const` by default.

### Capture list
```cpp
[]        // capture nothing
[x]       // x by copy
[&x]      // x by reference
[=]       // everything used, by copy      <- avoid: hides what you captured
[&]       // everything used, by reference <- avoid in anything stored
[this]    // the enclosing object by POINTER (captures `this`, not *this!)
[*this]   // C++17: a COPY of the enclosing object
[x = expr]          // C++14 init-capture: create a new member
[p = std::move(p)]  // move a unique_ptr into the lambda
[...args = std::forward<Args>(args)]   // C++20 pack init-capture
```

The three traps:

```cpp
// 1. Dangling capture by reference — the #1 async bug.
std::function<int()> make() {
    int local = 42;
    return [&local] { return local; };      // dangles the moment make() returns
}

// 2. [this] in an async callback dangles if the object dies first.
void Sensor::start() {
    pool.post([this] { process(buffer_); });   // outlives *this? use-after-free
}
// fix: capture a shared_ptr, or a weak_ptr you lock, or copy what you need:
void Sensor::start() {
    pool.post([self = shared_from_this()] { self->process(self->buffer_); });
}

// 3. mutable, and the const-by-default operator()
auto counter = [n = 0]() mutable { return ++n; };   // `mutable` required to
                                                    // modify a by-copy capture
```

### Conversion to a function pointer
A **captureless** lambda converts implicitly to a plain function pointer — that is
how you pass one to a C API:
```cpp
auto cmp = [](const void* a, const void* b) { /* ... */ return 0; };
std::qsort(data, n, sizeof(int), cmp);        // works: no captures
```

### Other specifiers
```cpp
[](auto x) { return x; }                       // C++14 generic lambda (templated op())
[]<typename T>(T x) { return x; }              // C++20 explicit template param
[]() constexpr { return 1; }                   // usable in constant expressions
[]() noexcept { }
[]() -> decltype(auto) { }                     // preserve references in the return
auto rec = [](auto&& self, int n) -> int {     // recursion pre-C++23
    return n <= 1 ? 1 : n * self(self, n - 1);
};
// C++23: [](this auto&& self, int n) { ... }  // explicit object parameter
```

Lambdas are the idiomatic way to pass behaviour into an algorithm, to build a
scope guard, and to make a complex `const` initialization one expression:
```cpp
const auto table = [] { std::array<int, 256> t{}; /* fill */ return t; }();
```

## 5. The preprocessor — know it, avoid it

```cpp
#define SQUARE(x) ((x) * (x))     // parenthesize EVERY parameter and the whole body
SQUARE(1 + 2)                     // without the parens: 1 + 2 * 1 + 2 == 5
int i = 0; SQUARE(i++);           // double evaluation -> UB. constexpr fn instead.

#define MIN(a,b) ((a) < (b) ? (a) : (b))   // still double-evaluates. Use std::min.
```

Rule: **a `constexpr` (or `inline`) function instead of a function-like macro,
always.** Macros ignore scope and namespaces, break the debugger, break the type
system, and are the reason `min`/`max` on Windows require `NOMINMAX`.

Legitimate uses that remain:
- include guards (`#pragma once` is not standard but is universally supported and
  faster)
- conditional compilation on platform/feature (`#ifdef __AVX2__`,
  `#if __cpp_lib_span >= 202002L`, `__has_include`)
- stringizing for logging/asserts — `#` and `##` cannot be done any other way
- source location before C++20 (`__FILE__`, `__LINE__`, `__func__`) — now
  `std::source_location`

```cpp
#define STR(x) #x                       // stringize
#define XSTR(x) STR(x)                  // expand first, THEN stringize
#define CONCAT(a, b) a##b               // token pasting
#define CHECK(cond) \
    do { if (!(cond)) fail(#cond, __FILE__, __LINE__); } while (0)
//  ^ the do/while(0) makes the macro a single statement that needs a semicolon
#define LOG(fmt, ...) std::printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define LOG0(fmt, ...) std::printf(fmt __VA_OPT__(,) __VA_ARGS__)   // C++20
```

Feature detection, which is worth knowing by name:
```cpp
#if __has_include(<expected>)
#  include <expected>
#endif
#if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 201911L
// ...
#endif
#if defined(__GNUC__) && !defined(__clang__)
#endif
```

Modern replacement for `__FILE__`/`__LINE__`:
```cpp
void log(std::string_view msg,
         std::source_location loc = std::source_location::current()) {
    std::printf("%s:%u %s\n", loc.file_name(), loc.line(), msg.data());
}
```
Default arguments are evaluated at the call site, which is exactly why this works.

---

## Traps checklist

1. `std::string_view` / `std::span` do not own — never outlive the buffer.
2. Return type is not part of the overload signature; top-level `const` on a
   by-value parameter is ignored.
3. Default arguments belong in the declaration only, and are evaluated at the
   call site.
4. `[&]` in anything stored or posted to a queue = dangling capture.
5. `[this]` captures the pointer, not the object. `[*this]` copies.
6. Lambda `operator()` is `const` unless you say `mutable`.
7. Captureless lambdas convert to function pointers; capturing ones do not.
8. `std::function` may allocate and always costs an indirect call — use a
   template parameter in hot code.
9. Function-like macros double-evaluate arguments; use `constexpr` functions.
10. `#` and `##` are the only things macros can do that the language cannot.
