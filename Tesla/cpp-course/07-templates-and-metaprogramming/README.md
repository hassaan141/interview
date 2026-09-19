# 07 — Templates and Metaprogramming: Function & Class Templates, Traits, SFINAE, Concepts

Course chapters: **11 (Templates I), 12 (Templates II)**

Foundations code *is* templates. You will be asked to write a generic utility and
then to constrain it.

---

## 1. Function templates

```cpp
template <typename T>
constexpr const T& max_of(const T& a, const T& b) { return a < b ? b : a; }

max_of(1, 2);            // T deduced as int
max_of<double>(1, 2.5);  // explicit: both convert to double
// max_of(1, 2.5);       // ERROR: T deduced as int from arg1 and double from arg2
```

**Instantiation** happens on use; a template that is never instantiated is barely
checked (only non-dependent constructs are). This is why template errors appear at
the call site and are enormous.

Template parameters come in three flavours:
```cpp
template <typename T>              // type parameter
template <std::size_t N>           // non-type template parameter (NTTP): integral,
                                   // enum, pointer/reference to object with linkage,
                                   // nullptr_t, and since C++20 floating point and
                                   // literal class types
template <typename T, T Value>     // NTTP whose type depends on another parameter
template <template <typename> class Container>   // template template parameter
template <typename... Ts>          // parameter pack
template <auto N>                  // C++17: deduce the NTTP's type
```

Defaults, overloading, specialization:
```cpp
template <typename T, typename Alloc = std::allocator<T>> class Vec;   // default

template <typename T> void f(T);        // (1) primary
template <typename T> void f(T*);       // (2) an OVERLOAD, more specialized
template <>           void f(int);      // (3) explicit FULL specialization of (1)
void f(double);                          // (4) a plain function: wins any tie
```

**Function templates cannot be partially specialized** — only fully. Use
overloading (which participates in partial ordering) or tag dispatch or
`if constexpr` instead. Class templates *can* be partially specialized. This
distinction is a standard interview question.

Beware: specializations do not participate in overload resolution until after the
primary template is chosen, so `template<> void f(int)` specializes (1) even if (2)
would have been a better match. Prefer overloads over specializations for
functions.

## 2. Class templates

```cpp
template <typename T, std::size_t N>
class Array {
    T data_[N]{};
public:
    constexpr std::size_t size() const { return N; }
    constexpr T& operator[](std::size_t i) { return data_[i]; }
};

template <typename T> class Array<T, 0> {};              // partial specialization
template <>           class Array<bool, 8> { /* ... */ }; // full specialization
```

Members of a class template are themselves instantiated **lazily** — only the ones
you call. So a `std::vector<T>` where `T` is not comparable still compiles until
you call `std::sort` on it.

**CTAD — class template argument deduction** (C++17):
```cpp
std::vector v{1, 2, 3};             // std::vector<int>
std::pair p{1, 2.0};                // std::pair<int, double>
std::lock_guard lk{m};              // std::lock_guard<std::mutex>

template <typename T> struct Wrapper { T v; };
Wrapper w{1.5};                     // Wrapper<double>, via the implicit guide
template <typename T> Wrapper(T) -> Wrapper<T>;          // explicit deduction guide
template <typename T> Wrapper(std::initializer_list<T>) -> Wrapper<std::vector<T>>;
```

### Dependent names: `typename` and `template`
Inside a template, a name that depends on a template parameter is a **dependent
name**, and the compiler does not know whether it is a type or a value. You must
tell it:
```cpp
template <typename T>
void f() {
    typename T::value_type x{};              // `typename`: it IS a type
    T::template rebind<int> y;               // `template`: it IS a template
    typename T::template rebind<int>::other z;
}
```
(C++20 relaxed some cases where `typename` is now optional; write it anyway.)

### Two-phase lookup / a base class that depends on `T`
```cpp
template <typename T> struct Base { void helper(); int value; };
template <typename T> struct Derived : Base<T> {
    void f() {
        // helper();                 // ERROR: not found — Base<T> is dependent
        this->helper();              // OK
        Base<T>::helper();           // OK
        using Base<T>::value;        // OK (in a using-declaration)
    }
};
```

## 3. Compile-time utilities

```cpp
static_assert(cond, "message");             // message optional since C++17
using Alias = std::vector<int>;              // prefer `using` over `typedef`
template <typename T> using Vec = std::vector<T>;   // alias TEMPLATE — typedef can't

decltype(expr)          // the declared type of an expression, refs and cv preserved
decltype(auto) f();     // deduce the return type WITH references (unlike auto)
std::declval<T>()       // a fake T&& for use in unevaluated contexts
```

`decltype` rules worth knowing: `decltype(x)` for a variable gives its declared
type; `decltype((x))` — with extra parens — gives `T&` because `(x)` is an lvalue
expression. That parenthesis is a real interview question.

## 4. Type traits

```cpp
#include <type_traits>
std::is_integral_v<T>          std::is_floating_point_v<T>
std::is_pointer_v<T>           std::is_reference_v<T>
std::is_same_v<T, U>           std::is_base_of_v<B, D>
std::is_convertible_v<F, T>    std::is_constructible_v<T, Args...>
std::is_trivially_copyable_v<T>  std::is_nothrow_move_constructible_v<T>
std::is_invocable_v<F, Args...>  std::is_invocable_r_v<R, F, Args...>

std::remove_reference_t<T>     std::remove_cv_t<T>     std::decay_t<T>
std::remove_cvref_t<T>         // C++20: the one you usually want
std::add_pointer_t<T>          std::underlying_type_t<Enum>
std::conditional_t<B, T, F>    std::enable_if_t<B, T>
std::invoke_result_t<F, Args...>
std::common_type_t<Ts...>
```

Writing one yourself (they ask this):
```cpp
template <typename T> struct is_pointer_impl            : std::false_type {};
template <typename T> struct is_pointer_impl<T*>        : std::true_type {};
template <typename T> struct is_pointer_impl<T* const>  : std::true_type {};
template <typename T>
constexpr bool is_pointer_v2 = is_pointer_impl<std::remove_cv_t<T>>::value;
```
The pattern: a primary template with the default answer, plus partial
specializations that pattern-match the structure of the type.

## 5. Template metaprogramming → `constexpr` → `consteval`

The historical way (recursive instantiation) vs. what you should write now:
```cpp
// TMP: one class instantiation per value. Slow to compile, awful errors.
template <unsigned N> struct Factorial { static constexpr unsigned value = N * Factorial<N-1>::value; };
template <>           struct Factorial<0> { static constexpr unsigned value = 1; };
static_assert(Factorial<5>::value == 120);

// Modern: just write the function.
constexpr unsigned factorial(unsigned n) { return n <= 1 ? 1 : n * factorial(n - 1); }
static_assert(factorial(5) == 120);
```
Say this in the interview: **"since C++14/17 most metaprogramming is just
`constexpr` functions and `if constexpr`; I use traits and concepts for
constraints, not for computation."**

## 6. SFINAE → concepts

**SFINAE** (Substitution Failure Is Not An Error): if substituting template
arguments produces an invalid type or expression **in the immediate context of the
signature**, that candidate is silently removed from the overload set instead of
being a hard error. Failures in the *body* are hard errors.

```cpp
// SFINAE, the C++11/14 way — three spellings you should recognise:
template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
void f(T);                                                   // extra NTTP
template <typename T> std::enable_if_t<std::is_integral_v<T>> g(T);   // return type
template <typename T> auto h(T t) -> decltype(t.size(), void()) {}    // expression
                                                             // SFINAE ("has size()")

// Tag dispatch, the other pre-concepts tool:
template <typename It> void advance_impl(It& it, int n, std::random_access_iterator_tag) { it += n; }
template <typename It> void advance_impl(It& it, int n, std::input_iterator_tag) { while (n--) ++it; }
```

**C++20 concepts** replace all of that — and give you readable errors:
```cpp
template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template <typename T>
concept Container = requires(T c) {
    typename T::value_type;                    // a type requirement
    c.begin();                                 // a simple requirement (must compile)
    c.end();
    { c.size() } -> std::convertible_to<std::size_t>;   // compound: type constraint
    requires std::is_default_constructible_v<T>;         // a nested requirement
};

template <Arithmetic T> T twice(T v) { return v + v; }         // constrained param
void f(Container auto& c);                                     // abbreviated
template <typename T> requires Arithmetic<T> T thrice(T v);     // requires clause
template <typename T> T quad(T v) requires Arithmetic<T>;       // trailing requires

// Overload on constraints — more-constrained wins (subsumption):
template <std::integral T>       int which(T) { return 1; }
template <std::floating_point T> int which(T) { return 2; }
```

**Subsumption gotcha you will hit in practice:** subsumption compares the
*normalized* form of a constraint, which is a tree of **atomic constraints**. A
concept defined over a type trait is a single opaque atom:

```cpp
template <typename T> concept ArithTrait = std::is_arithmetic_v<T>;   // one atom

template <ArithTrait T>    int f(T);
template <std::integral T> int f(T);
f(1);   // AMBIGUOUS — the compiler cannot see that integral implies ArithTrait

template <typename T> concept ArithCpt = std::integral<T> || std::floating_point<T>;
template <ArithCpt T>      int g(T);
template <std::integral T> int g(T);
g(1);   // OK — std::integral is one term of the disjunction, so it subsumes it
```
Rule: **build concepts out of other concepts**, not out of `is_*_v` traits, if you
want them to order against each other.

Standard library concepts to know by name: `std::same_as`, `std::convertible_to`,
`std::integral`, `std::floating_point`, `std::totally_ordered`, `std::invocable`,
`std::predicate`, `std::copyable`, `std::movable`, `std::ranges::range`,
`std::ranges::random_access_range`, `std::sized_sentinel_for`.

`requires requires` — you will see it, and it is not a typo: the first introduces
the clause, the second introduces an ad-hoc expression.
```cpp
template <typename T> requires requires(T t) { t.serialize(); }
void save(const T& t);
```

## 7. Variadic templates and fold expressions

```cpp
template <typename... Ts> struct TypeList {};
template <typename... Ts>
constexpr std::size_t count = sizeof...(Ts);

// Pre-C++17 recursion:
template <typename T> T sum(T v) { return v; }
template <typename T, typename... Rest> T sum(T first, Rest... rest) {
    return first + sum(rest...);
}

// C++17 fold expressions — prefer these:
template <typename... Ts> auto add(Ts... vs) { return (vs + ...); }        // unary right
template <typename... Ts> auto add0(Ts... vs) { return (0 + ... + vs); }   // binary left
template <typename... Ts> bool all_true(Ts... vs) { return (vs && ...); }
template <typename... Ts> void print(const Ts&... vs) {
    ((std::cout << vs << ' '), ...);                       // comma fold
}
// Perfect forwarding a pack:
template <typename... Args> auto make(Args&&... args) {
    return T(std::forward<Args>(args)...);
}
```
Empty-pack identities: `(... && vs)` is `true`, `(... || vs)` is `false`,
`(vs + ...)` on an empty pack is ill-formed (only `&&`, `||`, and `,` have
identities).

Homogeneous packs, cleanly:
```cpp
template <std::same_as<int>... Ts> int sum_ints(Ts... vs) { return (vs + ...); }
void f(std::initializer_list<int> vs);        // simpler when the type is fixed
```

## 8. Debugging templates

1. **Make the compiler print the type**: an undefined class template with the type
   as an argument.
   ```cpp
   template <typename...> struct WhatIs;
   WhatIs<decltype(expr)> _;     // error message contains the full type
   ```
2. `static_assert(std::is_same_v<A, B>)` to pin a deduction.
3. `-fconcepts-diagnostics-depth=3`, `-ftemplate-backtrace-limit=0`.
4. **Constrain early**: a concept turns a 200-line instantiation backtrace into one
   line naming the unsatisfied requirement. That is the real argument for concepts.
5. `__PRETTY_FUNCTION__` / `std::source_location::function_name()` inside a
   template prints the deduced arguments at runtime.

---

## Traps checklist

1. Function templates cannot be **partially** specialized — use overloads,
   tag dispatch, or `if constexpr`.
2. Deduction does not do conversions: `max_of(1, 2.5)` fails.
3. Dependent names need `typename` / `template`.
4. Names in a dependent base need `this->` or explicit qualification.
5. `decltype(x)` vs. `decltype((x))` — the parens make it a reference.
6. SFINAE only applies in the immediate context of the signature; body errors are
   hard errors.
7. Prefer `constexpr` functions over recursive class-template computation.
8. `(vs + ...)` on an empty pack is ill-formed; `&&`, `||`, `,` have identities.
9. `std::remove_cvref_t` is almost always the trait you want in a forwarding
   context, not `remove_reference_t`.
10. Concepts subsume: the more-constrained overload wins.
