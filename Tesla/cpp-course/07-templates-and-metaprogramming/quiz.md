# 07 — Mock interview questions

## A. Rapid fire

1. When is a template instantiated? What is checked before that?
2. Name the kinds of template parameters (there are five).
3. Can a function template be partially specialized? What do you do instead?
4. Why does `max_of(1, 2.5)` fail to compile? Three ways to fix it.
5. Why does `template <> void f(int)` sometimes lose to an overload you did not expect?
6. What is CTAD and what is a deduction guide?
7. Why does `typename T::value_type` need `typename`?
8. Why does `helper()` not compile in a class derived from `Base<T>`?
9. `decltype(x)` vs `decltype((x))` — what is the difference and why?
10. `auto` vs `decltype(auto)` as a return type.
11. What is `std::declval` for? Why can you never call it?
12. Write `is_pointer` from scratch. What is the general pattern for a trait?
13. What does `std::void_t` do in the detection idiom?
14. `std::decay_t` vs `std::remove_cvref_t` — which do you want and when?
15. Spell out SFINAE. Where does it apply and where does it not?
16. Give the three classic `enable_if` spellings.
17. What is tag dispatch and what replaced it?
18. What are the four kinds of requirement inside a `requires { }` block?
19. What does "more constrained wins" mean and when does it silently fail?
20. What does `requires requires` mean?
21. What is a fold expression? Which operators have an empty-pack identity?
22. Name three techniques for debugging a template error.
23. Why does constraining a template improve the error message *and* the build time?

## B. Find the bug / what does it do

**B1.**
```cpp
template <typename T> void process(T& out, const T& in);
std::vector<int> v; const std::vector<int> c;
process(v, c);              // ?
```

**B2.**
```cpp
template <typename Container>
void print(const Container& c) {
    Container::value_type first = *c.begin();
    std::cout << first;
}
```

**B3.**
```cpp
template <typename T> struct Stack {
    void push(const T&);
    void sort() { std::sort(data_.begin(), data_.end()); }
    std::vector<T> data_;
};
Stack<std::function<void()>> s;      // compiles?
s.push([]{});                        // compiles?
```

**B4.**
```cpp
template <typename T> T divide(T a, T b) { return a / b; }
divide(1, 0);
divide(1.0, 0.0);
```

**B5.**
```cpp
template <typename T> void f(T&& x) { g(x); }          // forwarding intended
```

**B6.**
```cpp
template <typename T> struct Registry {
    static std::vector<T> items;
};
template <typename T> std::vector<T> Registry<T>::items;
// used from two translation units -- one instance or two?
```

**B7.**
```cpp
template <typename T>
concept HasSize = requires(T t) { t.size(); };     // checked on a NON-const T
struct S { int size(); };                          // size() is non-const
template <HasSize T> std::size_t f(const T& t) { return t.size(); }
const S s{};
f(s);                                              // ?
```

**B8.**
```cpp
template <typename... Ts> auto product(Ts... vs) { return (vs * ...); }
product();
```

## C. Whiteboard

**C1.** Write a `constexpr` `tuple_for_each(tuple, fn)` that calls `fn` on every
element, using an index sequence. Then do it with a fold expression.

**C2.** Write a constrained `template <Numeric T> class Statistics` that
accumulates count/mean/variance with Welford, rejects non-arithmetic `T` with a
readable error, and is `constexpr`-usable. Define the `Numeric` concept yourself.

**C3.** Write `to_string_any(T)` that dispatches: arithmetic → `std::to_string`,
anything streamable → `ostringstream`, a range → `[a, b, c]`, anything else →
a compile error naming the problem. Do it with `if constexpr` and concepts.

**C4.** Your build takes 40 minutes and one template-heavy header is the cause.
Name six concrete things you would do, in order of expected payoff.

---
---

# Answers

**A1.** On **use** (implicit instantiation) — or at an explicit instantiation
(`template class Vec<int>;`). Before that, only non-dependent constructs are
checked: syntax, and names that do not depend on a template parameter. Members of a
class template are instantiated **lazily**, one member at a time, only when used.

**A2.** Type parameters (`typename T`), non-type template parameters
(`std::size_t N` — integral, enum, pointer/reference/member-pointer with linkage,
`nullptr_t`, and since C++20 floating point and literal class types), template
template parameters (`template <typename> class C`), parameter packs
(`typename... Ts`), and `auto` NTTPs (`template <auto N>`, C++17).

**A3.** **No** — only fully (explicitly) specialized. Instead: add an **overload**
(overloads participate in partial ordering, so `f(T*)` is more specialized than
`f(T)`), use **tag dispatch**, use `if constexpr` inside one function, or delegate
to a partially specializable **class** template.

**A4.** `T` is deduced independently from each argument — `int` from `1`,
`double` from `2.5` — and deduction never applies implicit conversions, so it is a
conflict. Fixes: specify it (`max_of<double>(1, 2.5)`), use two template parameters
plus `std::common_type_t` for the return, or make one parameter non-deduced
(`template <typename T> const T& max_of(const T&, std::type_identity_t<const T&>)`).

**A5.** Because explicit specializations do **not** participate in overload
resolution. The compiler first picks the best *primary* template (or non-template),
and only then looks for a specialization of the one it chose. So
`template <> void f(int)` specializing `f(T)` is ignored if `f(T*)` or a plain
`f(double)` was selected. Guidance: **overload function templates, do not specialize
them.**

**A6.** CTAD lets you write `std::vector v{1,2,3}` and have the class template
arguments deduced from the constructor arguments. The compiler synthesizes implicit
guides from the constructors; a **deduction guide**
(`template <typename T> Wrapper(T) -> Wrapper<T>;`) lets you add or correct them —
necessary when a constructor takes an iterator pair, an `initializer_list`, or when
you want to decay/transform the deduced type.

**A7.** Because `T::value_type` is a **dependent** name and the compiler must decide
at parse time whether it names a type or a value (the grammar differs —
`T::x * p;` is either a declaration or a multiplication). The default assumption is
"not a type", so you write `typename` to say otherwise. Same reason `->template
rebind<U>` needs `template`.

**A8.** Unqualified name lookup does not search **dependent** base classes, because
`Base<T>` is not known until instantiation and a specialization could change what
is in it. Fix with `this->helper()`, `Base<T>::helper()`, or a
`using Base<T>::helper;` declaration.

**A9.** `decltype(x)` where `x` is a variable name gives its **declared type**
(`int`). `decltype((x))` treats `(x)` as an **expression**; a named variable used as
an expression is an lvalue, and `decltype` of an lvalue expression of type `T` is
`T&` — so you get `int&`. This is why `decltype(auto)` return types are sensitive to
whether you write `return x;` or `return (x);` (the latter returns a dangling
reference to a local).

**A10.** `auto` deduces by template-argument-deduction rules: it **decays**, so
references and top-level cv are dropped. `decltype(auto)` deduces by `decltype`
rules: references and cv are **preserved** exactly. Use `decltype(auto)` for
perfect-forwarding wrappers whose return type must match the wrapped call exactly.

**A11.** `std::declval<T>()` produces an expression of type `T&&` for use in
**unevaluated** contexts (`decltype`, `sizeof`, `noexcept`, trailing return types) so
you can ask "what would this expression's type be" without needing a
default-constructible `T`. It has no definition, so calling it at runtime is a link
error by design.

**A12.**
```cpp
template <typename T> struct is_pointer_      : std::false_type {};
template <typename T> struct is_pointer_<T*>  : std::true_type  {};
```
Pattern: a **primary template giving the default answer**, plus **partial
specializations that pattern-match the structure of the type**, inheriting from
`std::true_type`/`false_type` (which supply `::value`, `::type`, and a conversion).
Remember to handle cv-qualification, usually by normalizing with
`std::remove_cv_t` before dispatching.

**A13.** `std::void_t<Ts...>` is an alias for `void` that *fails substitution* if
any `Ts` is ill-formed. In the detection idiom it is the SFINAE trigger: the partial
specialization `has_size<T, std::void_t<decltype(declval<T&>().size())>>` only
matches if that `decltype` is valid; otherwise the primary template (`false_type`)
is used. C++20 replaces the whole idiom with `requires`.

**A14.** `std::decay_t` removes references and cv **and** applies array-to-pointer
and function-to-pointer decay — it models what pass-by-value does.
`std::remove_cvref_t` (C++20) removes only references and cv. In a
forwarding-reference context (`template <typename T> void f(T&&)`) where you want
the underlying type without decaying arrays, `remove_cvref_t` is what you want;
`decay_t` is right when you are about to store the value.

**A15.** *Substitution Failure Is Not An Error.* When template arguments are
substituted into a candidate's **signature** — template parameter list, return
type, parameter types, and (C++20) its constraints — an invalid type or expression
removes that candidate from the overload set silently. Failures in the **body** (the
"non-immediate context") are hard errors. That asymmetry is the whole subtlety.

**A16.** (1) Extra defaulted template parameter:
`template <typename T, std::enable_if_t<C<T>, int> = 0> void f(T);`
(2) Return type: `template <typename T> std::enable_if_t<C<T>> f(T);`
(3) Trailing-return/expression SFINAE:
`template <typename T> auto f(T t) -> decltype(t.size(), void());`
Also (4) a defaulted function parameter, which is the worst of the four.

**A17.** Passing an empty tag type to select among overloads based on a trait —
`advance_impl(it, n, typename std::iterator_traits<It>::iterator_category{})`. It
was the pre-concepts way to get *positive* dispatch (rather than `enable_if`'s
negative filtering). C++17 `if constexpr` replaced it for single functions, and
C++20 constrained overloads replaced it in general.

**A18.** (1) **Simple**: an expression that must be valid (`c.begin();`).
(2) **Type**: `typename T::value_type;`.
(3) **Compound**: `{ expr } noexcept -> ConceptName;` — expression valid, optionally
non-throwing, result satisfying a type constraint.
(4) **Nested**: `requires OtherConcept<T>;` — a boolean constraint.

**A19.** When two candidates both satisfy their constraints and one's constraints
**subsume** the other's, the more-constrained one wins instead of being ambiguous.
It silently fails when the constraints are not *related* after normalization — in
particular, a concept written as a single opaque atom over a type trait
(`concept A = std::is_arithmetic_v<T>;`) does not subsume or get subsumed by
`std::integral`, so you get an ambiguity error instead of the ordering you
expected. Build concepts out of concepts.

**A20.** The first `requires` introduces a *requires-clause* (a constraint on the
template); the second introduces a *requires-expression* (an ad-hoc, unnamed
concept). `template <typename T> requires requires(T t) { t.serialize(); } void
save(const T&);` — it is legal and it is what you write when the constraint is not
worth naming. Naming it is usually better.

**A21.** A pack expansion over a binary operator: unary right `(vs op ...)`, unary
left `(... op vs)`, binary right `(vs op ... op init)`, binary left
`(init op ... op vs)`. Only `&&` (identity `true`), `||` (identity `false`), and
`,` (identity `void()`) are valid on an **empty** pack; everything else, including
`+` and `*`, is ill-formed — use a binary fold with an explicit init value.

**A22.** (1) An undefined class template as a type printer:
`template <typename...> struct WhatIs; WhatIs<decltype(e)> probe;` — the error
message contains the fully spelled type. (2) `static_assert(std::is_same_v<A,B>)` to
pin a deduction at the point you care about. (3) Constrain with concepts so the
failure is reported at the *interface* instead of 200 frames deep;
plus `-fconcepts-diagnostics-depth=`, `-ftemplate-backtrace-limit=0`,
`__PRETTY_FUNCTION__`, and C++20 `static_assert` with a `constexpr` message.

**A23.** Error message: an unsatisfied constraint is reported once, at the call
site, naming the requirement that failed — instead of the compiler instantiating the
body and reporting the first thing that broke deep inside. Build time: a constrained
overload is rejected during *constraint checking*, before its body is instantiated,
so the compiler does far less template instantiation work — and concepts are cached
by the compiler, unlike `enable_if` SFINAE which re-substitutes the whole signature
for every candidate.

---

**B1.** `T` deduces to `std::vector<int>` from the first argument and
`std::vector<int>` from the second (the `const` is absorbed by `const T&`), so it
compiles. But if you wrote `process(c, v)` it would fail — `T&` binding to a
`const` object deduces `T = const std::vector<int>`, which then conflicts with the
non-const second argument. The fix is two template parameters; the lesson is that
one `T` in two positions couples them.

**B2.** Needs `typename`: `typename Container::value_type first = ...`. Also it
should be `const auto& first` to avoid a copy, and calling `*c.begin()` on an empty
container is UB.

**B3.** `Stack<std::function<void()>>` **compiles** (class template members are
instantiated lazily), and `push` compiles too. Only `s.sort()` would fail —
`std::function` is not `<`-comparable — and the error would appear deep inside
`std::sort`. This is the lazy-instantiation property, and the reason to constrain
`sort` with `requires std::totally_ordered<T>` so the error names the real problem.

**B4.** `divide(1, 0)` is integer division by zero → **undefined behavior** (SIGFPE
in practice). `divide(1.0, 0.0)` is well-defined IEEE-754 → `+inf`. Same source
line, two completely different semantics depending on the deduced `T` — which is
exactly why generic numeric code needs either a constraint plus a precondition
check, or separate integral/floating overloads.

**B5.** `g(x)` passes `x` as an **lvalue** regardless of how it was called, so the
forwarding is lost: an rvalue argument gets copied instead of moved, and `g`'s
rvalue overload is never selected. Fix: `g(std::forward<T>(x));`. (Also
`std::forward` must be given the template parameter explicitly — `std::forward(x)`
is wrong.)

**B6.** **One** instance per distinct `T` across the whole program. A static data
member of a class template is an implicitly instantiated template entity: the linker
merges the definitions from every TU (it goes in a COMDAT/weak section), so
`Registry<int>::items` is a single object. That is also the hazard — it is global
mutable state with a non-obvious initialization order, so it is not thread-safe and
it participates in the static-initialization-order fiasco.

**B7.** The **constraint is satisfied but the body fails to compile** — the worst of
both worlds. `HasSize<S>` is checked against a *non-const* `S`, where `size()` is
callable, so the overload is selected; then the body calls `t.size()` on a
`const S&` and you get
`error: passing 'const S' as 'this' argument discards qualifiers` from *inside* the
template — exactly the error concepts were supposed to move to the call site.
Fix: write the concept against the value category and constness the body actually
uses, `concept HasSize = requires(const T& t) { t.size(); };`. This is the single
most common concept-authoring mistake, and it is why you write the concept by
copying the expressions out of the body.

**B8.** Ill-formed. `*` has no identity for an empty pack, so `product()` does not
compile. Use a binary fold with an explicit init: `(1 * ... * vs)`.

---

**C1.**
```cpp
// (a) index_sequence + fold
template <typename Tuple, typename F, std::size_t... Is>
constexpr void for_each_impl(Tuple&& t, F&& f, std::index_sequence<Is...>) {
    (f(std::get<Is>(std::forward<Tuple>(t))), ...);        // comma fold
}
template <typename Tuple, typename F>
constexpr void tuple_for_each(Tuple&& t, F&& f) {
    for_each_impl(std::forward<Tuple>(t), std::forward<F>(f),
                  std::make_index_sequence<
                      std::tuple_size_v<std::remove_cvref_t<Tuple>>>{});
}
// (b) std::apply hides the index_sequence entirely
template <typename Tuple, typename F>
constexpr void tuple_for_each2(Tuple&& t, F&& f) {
    std::apply([&f](auto&&... elems) { (f(decltype(elems)(elems)), ...); },
               std::forward<Tuple>(t));
}
// usage
tuple_for_each(std::tuple{1, 2.5, 'c'}, [](const auto& v) { std::cout << v << ' '; });
```
Points to volunteer: `remove_cvref_t` before `tuple_size_v` (a forwarded `Tuple` is
a reference); the comma fold guarantees **left-to-right order**, which a
`std::initializer_list` trick also gives you but a plain pack expansion does not;
`std::apply` is the idiomatic answer and shows you know the library.

**C2.**
```cpp
template <typename T>
concept Numeric = (std::integral<T> || std::floating_point<T>) && !std::same_as<T, bool>;

template <Numeric T>
class Statistics {
    std::size_t n_{0};
    T mean_{}, m2_{};
public:
    constexpr void add(T x) noexcept {
        ++n_;
        const T delta = x - mean_;
        mean_ += delta / static_cast<T>(n_);
        m2_   += delta * (x - mean_);          // uses the UPDATED mean
    }
    constexpr std::size_t count() const noexcept { return n_; }
    constexpr T mean() const noexcept { return mean_; }
    constexpr T variance() const noexcept {           // population
        return n_ ? m2_ / static_cast<T>(n_) : T{};
    }
    constexpr T sample_variance() const noexcept {
        return n_ > 1 ? m2_ / static_cast<T>(n_ - 1) : T{};
    }
};
static_assert([] { Statistics<double> s; s.add(1); s.add(2); s.add(3);
                   return s.mean() == 2.0 && s.variance() == 2.0/3.0; }());
```
Say out loud: excluding `bool` (which is `std::integral`) because the statistics are
meaningless; Welford so there is no `E[x²] − E[x]²` cancellation (see section 01);
`constexpr` so the unit test is a `static_assert`; and that for an *integral* `T` the
division truncates, so a real implementation would accumulate in
`std::conditional_t<std::floating_point<T>, T, double>` — mentioning that
unprompted is the difference between a passing and a strong answer.

**C3.**
```cpp
template <typename T>
concept Streamable = requires(std::ostream& os, const T& t) { os << t; };

template <typename T>
std::string to_string_any(const T& v) {
    if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, char>) {
        return std::to_string(v);
    } else if constexpr (std::convertible_to<T, std::string_view>) {
        return std::string{std::string_view{v}};
    } else if constexpr (std::ranges::range<T>) {
        std::string out = "[";
        bool first = true;
        for (const auto& e : v) {
            if (!first) out += ", ";
            out += to_string_any(e);                 // recurses: nested ranges work
            first = false;
        }
        return out + "]";
    } else if constexpr (Streamable<T>) {
        std::ostringstream os; os << v; return os.str();
    } else {
        static_assert(false_v<T>,                    // a dependent false
            "to_string_any: T is not arithmetic, a string, a range, or streamable");
    }
}
template <typename> constexpr bool false_v = false;   // needed pre-C++23
```
Points: order the branches from most specific to least (a `std::string` is *both* a
range and streamable, so it must be tested first); the `static_assert` must depend
on `T` (`false_v<T>`, not plain `false`) or it fires even when the branch is
discarded — C++23 relaxed this; recursion handles nested containers; `if constexpr`
rather than overloads keeps it one readable function.

**C4.** In order of expected payoff:
1. **Measure first**: `clang -ftime-trace` + `ChromeTracing`, or gcc
   `-ftime-report`, and `include-what-you-use` / `clang-include-graph`. Do not guess
   which header it is — and confirm whether the cost is *parsing* headers or
   *instantiating* templates, because the fixes differ.
2. **Cut the include graph**: forward declarations, move implementation-only
   includes into the `.cpp`, PIMPL for the worst offenders. This is usually the
   biggest single win because every includer pays.
3. **Stop instantiating what you don't need**: replace `std::function` in interfaces
   with a non-template callback type, replace deep `enable_if` chains with
   **concepts** (rejected before the body is instantiated), and replace recursive
   class-template computation with `constexpr` functions or fold expressions.
4. **`extern template`** for the handful of heavy instantiations used everywhere
   (`std::vector<MyType>`, a big `Matrix<double, 4, 4>`), instantiated once in a
   dedicated TU. Also hoist type-independent code out of templates into non-template
   base functions ("thin template" idiom).
5. **Build system**: `ninja` instead of `make`, `ccache`/`sccache`, a
   precompiled header for the stable third-party surface, unity builds for the
   coldest directories, `-fno-rtti`/`-fno-exceptions` where allowed, and a faster
   linker (`mold`/`lld`) since link time is often half the wall clock.
6. **Modules (C++20)** for genuinely new code, and split the monolithic header into
   fine-grained ones so a one-line change does not rebuild the world.
Then add the meta-point: track build time in CI as a metric with a regression
budget, or it silently comes back.
