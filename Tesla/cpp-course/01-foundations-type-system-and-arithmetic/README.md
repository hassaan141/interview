# 01 — Foundations: Type System, Integral & Floating-Point Arithmetic

Course chapters: **1 (Introduction), 2 (Preparation), 3 (Basic Concepts I),
4 (Basic Concepts II), 5 (Basic Concepts III)**

Why grouped: chapters 1-2 are context and setup; 3-5 are one topic — "what are
the built-in types and how does arithmetic on them actually behave". For an
autonomy role this is not trivia: signed overflow is UB the compiler exploits,
and float comparison bugs are how a controller ends up oscillating.

---

## 1. The C++ type system in one page

C++ types split into:

- **Fundamental**: `void`, `std::nullptr_t`, arithmetic types.
  - integral: `bool`, `char`/`signed char`/`unsigned char`, `char8_t`,
    `char16_t`, `char32_t`, `wchar_t`, `short`, `int`, `long`, `long long`
    (+ `unsigned` variants)
  - floating: `float`, `double`, `long double` (+ C++23 `std::float16_t` etc. in
    `<stdfloat>`)
- **Compound**: pointers, references, arrays, functions, enums, classes/structs,
  unions, pointers-to-member.

Type *properties* you will be asked about (all queryable in `<type_traits>`):

| Property | Means | Trait |
| --- | --- | --- |
| trivially copyable | `memcpy` of the object is a valid copy | `std::is_trivially_copyable_v<T>` |
| trivially destructible | destructor does nothing observable | `std::is_trivially_destructible_v<T>` |
| standard layout | member layout is C-compatible | `std::is_standard_layout_v<T>` |
| POD | trivial **and** standard layout | both of the above |
| aggregate | can be brace-initialized member-wise | `std::is_aggregate_v<T>` |

**Interview line:** "trivially copyable is what lets me `memcpy` a sensor frame
or put it in shared memory; standard layout is what lets C and C++ agree on the
offsets." That single sentence is why a foundations team cares.

### `sizeof` guarantees you can rely on

Guaranteed: `sizeof(char) == 1`, and
`1 == sizeof(char) <= sizeof(short) <= sizeof(int) <= sizeof(long) <= sizeof(long long)`.
`char` is **at least** 8 bits (`CHAR_BIT`), signedness of plain `char` is
implementation-defined. Nothing else is guaranteed — so for anything crossing an
ABI, a wire format, or a register map, use `<cstdint>`:

```cpp
#include <cstdint>
std::int32_t  x;   // exactly 32 bits, two's complement (C++20 mandates two's complement)
std::uint8_t  b;   // exactly 8 bits
std::int_fast32_t f;   // at least 32 bits, fastest such type
std::int_least16_t l;  // smallest type with >= 16 bits
std::size_t     n;     // unsigned, can hold any object size (result of sizeof)
std::ptrdiff_t  d;     // signed, result of pointer subtraction
std::uintptr_t  p;     // unsigned int big enough to hold a pointer
std::intmax_t   m;     // widest signed integer
```

`std::size_t` is 64-bit on LP64 Linux/x86-64 and aarch64; `long` is 64-bit on
LP64 but 32-bit on Windows LLP64. Never assume.

## 2. `auto`, and when *not* to use it

`auto` deduces using **template argument deduction** rules — meaning it
**decays**: it strips references, top-level `const`/`volatile`, and converts
arrays/functions to pointers.

```cpp
const std::vector<int> v{1,2,3};
auto  a = v;        // std::vector<int>          (copy! const dropped)
auto& r = v;        // const std::vector<int>&   (no copy)
const auto& cr = v; // const std::vector<int>&   (works for temporaries too)
auto&& u = v;       // const std::vector<int>&   (universal ref, binds anything)

int arr[5];
auto  p = arr;      // int*   (array decayed)
auto& q = arr;      // int(&)[5]  (no decay)
```

Two classic bugs:

```cpp
std::vector<bool> bits{true,false};
auto b = bits[0];        // NOT bool — it's std::vector<bool>::reference, a proxy!
bool b2 = bits[0];       // fine

std::map<std::string,int> m;
for (const std::pair<std::string,int>& kv : m) {}   // copies every key! value_type
                                                     // is pair<const string,int>
for (const auto& kv : m) {}                          // correct
for (const auto& [k, val] : m) {}                    // correct + readable (C++17)
```

Rule for this interview: **`auto` for iterators, lambdas, and
template-heavy expressions; spell the type out for scalars and API boundaries.**

## 3. Operators — the parts people get wrong

Precedence you must know without looking it up:

```
unary  >  * / %  >  + -  >  << >>  >  <=> > < <= > >=  >  == !=  >  &  >  ^  >  |  >  && > ||  > ?: > = > ,
```

Yes, `<<` binds *tighter* than comparison, which is why
`std::cout << a < b` is a compile error, and `a & b == c` means `a & (b == c)`.
**Parenthesize bit operations. Always.**

```cpp
int i = 5;
int a = i++;   // a == 5, i == 6   (postfix: returns old value, so it must copy)
int b = ++i;   // b == 7, i == 7   (prefix: returns the object itself, by reference)
```

For class types prefer `++it` over `it++`: postfix has to make a copy of the
iterator. On a hot loop that is free real estate.

### Evaluation order (C++17 tightened this)
- `a op= b`, `a[b]`, `a->*b`, `<<`, `>>`: **left operand evaluated before right**
  since C++17.
- Function arguments: order is still **unspecified**. `f(i++, i++)` is
  unspecified order (no longer UB in C++17, but still garbage — don't).
- `,` `&&` `||` `?:` have always been sequenced left-to-right with short-circuit.

### Spaceship `<=>` (C++20)
```cpp
struct Point {
    int x, y;
    auto operator<=>(const Point&) const = default;  // gives < <= > >=
    bool operator==(const Point&) const = default;   // == and != separately
};
```
Return types: `std::strong_ordering` (substitutable), `std::weak_ordering`
(equivalent but distinguishable, e.g. case-insensitive strings),
`std::partial_ordering` (floats — `NaN` is unordered).

## 4. Integral arithmetic — the UB minefield

**Signed overflow is undefined behavior. Unsigned overflow wraps (mod 2^N).**

This matters because the optimizer *assumes UB never happens*:

```cpp
// The compiler may legally delete this check entirely:
bool will_overflow(int x) { return x + 1 < x; }   // always false to the optimizer

// Correct, C++20:
#include <numeric>   // C++26: std::add_sat
int r;
if (__builtin_add_overflow(x, 1, &r)) { /* handle */ }   // gcc/clang builtin
```

Other integer UB / IB to name in an interview:
- shift by >= bit width, or by a negative amount → **UB**
- `INT_MIN / -1` and `INT_MIN % -1` → **UB** (overflows)
- division/modulo by zero → **UB**
- left-shifting a negative signed value → UB before C++20, well-defined in C++20
- signed → unsigned conversion is **not** UB, it is modular (well-defined)

### Integer promotion & the usual arithmetic conversions

Every arithmetic operand smaller than `int` is first **promoted to `int`**
(integral promotion). Then, for a binary op:

1. If either is `long double`/`double`/`float` → both become that.
2. Else both integral: promote both; if same signedness → larger rank wins.
3. Mixed sign: if the **unsigned** type's rank >= the signed type's rank → the
   signed operand converts to **unsigned**. Otherwise, if the signed type can
   represent every value of the unsigned type → both become signed. Otherwise
   both become the unsigned version of the signed type.

The bug this causes, every single time:

```cpp
std::vector<int> v;
for (int i = 0; i < v.size() - 1; ++i) {}   // v.size() is size_t (unsigned).
                                            // empty vector -> 0u - 1 == SIZE_MAX
                                            // loop runs ~forever / reads OOB
```
Fixes: `for (std::size_t i = 0; i + 1 < v.size(); ++i)`, or
`std::ssize(v)` (C++20, returns signed), or iterate with ranges.

```cpp
unsigned char a = 200, b = 200;
int c = a + b;              // 400 — both promoted to int, no wrap
unsigned char d = a + b;    // 144 — 400 truncated (well-defined, still a bug)

int  x = -1;
unsigned u = 1;
x < u;   // FALSE! x converts to 4294967295u
```

C++20 gives you safe comparisons — use them:
```cpp
#include <utility>
std::cmp_less(x, u);       // true — compares mathematical values
std::in_range<std::int8_t>(300);  // false
```

## 5. Floating point — IEEE-754 in the detail you need

`float` = 1 sign + 8 exponent + 23 mantissa (24 bits effective precision,
~7 decimal digits). `double` = 1 + 11 + 52 (53 effective, ~16 digits).

Value classes: **normal**, **denormal/subnormal** (exponent field all zeros —
gradual underflow; *very slow* on some hardware, often flushed to zero with
`-ffast-math` or FTZ/DAZ CPU flags), **zero** (`+0.0` and `-0.0`, which compare
equal but are distinguishable via `std::signbit` and `1.0/x`), **infinity**,
**NaN**.

NaN properties that get asked:
- `NaN != NaN` is **true**; every comparison with NaN is false except `!=`.
- Detect with `std::isnan(x)` (never `x != x` under `-ffast-math`, which assumes
  no NaNs and deletes the check).
- Quiet vs. signaling NaN; NaN propagates through arithmetic.

```cpp
#include <limits>
std::numeric_limits<double>::epsilon();      // 2.22e-16, ULP at 1.0
std::numeric_limits<double>::min();          // smallest positive NORMAL
std::numeric_limits<double>::denorm_min();   // smallest positive denormal
std::numeric_limits<double>::infinity();
std::numeric_limits<double>::quiet_NaN();
std::numeric_limits<double>::max();
```

### Arithmetic properties you lose
FP addition and multiplication are **commutative but not associative** and not
distributive:
```cpp
(0.1 + 0.2) + 0.3 != 0.1 + (0.2 + 0.3);   // both are "0.6", different bits
```
Consequence: **parallel/vectorized reductions give different results than serial
ones**, and that is not a bug. If someone asks "why did the result change when
we enabled multithreading in the perception pipeline?" — this is the answer.
Fix with a deterministic reduction tree, Kahan/Neumaier summation, or
`std::fma`.

### Catastrophic cancellation
Subtracting two nearly-equal numbers annihilates the significant digits:
```cpp
// bad
double var = sum_sq / n - mean * mean;          // can go negative!
// quadratic formula: for b^2 >> 4ac, (-b + sqrt(disc)) cancels
double x1 = (-b + std::sqrt(d)) / (2*a);        // bad root
double x2 = (2*c) / (-b - std::sqrt(d));        // stable form of the same root
```
Use Welford's algorithm for running mean/variance — good answer for any
"streaming telemetry" question.

### Comparing floats (know all three)
```cpp
bool eq_abs(double a, double b, double tol = 1e-9) {          // near zero
    return std::fabs(a - b) <= tol;
}
bool eq_rel(double a, double b, double tol = 1e-9) {          // large magnitudes
    return std::fabs(a - b) <= tol * std::max(std::fabs(a), std::fabs(b));
}
bool eq_mixed(double a, double b, double atol = 1e-12, double rtol = 1e-9) {
    double diff = std::fabs(a - b);
    return diff <= atol || diff <= rtol * std::max(std::fabs(a), std::fabs(b));
}
// ULP-based: how many representable doubles apart are they?
```
`eq_mixed` is the one to write on the whiteboard: absolute tolerance handles the
zero crossing, relative handles big values.

### FP undefined / surprising behavior
- `0.0/0.0`, `inf - inf` → NaN (not UB in IEEE mode, but often UB per the
  standard for `int` division).
- Casting a float to an integer type when the value does not fit → **UB**.
  Use `std::lround`, or clamp first.
- `-ffast-math` implies `-ffinite-math-only` + `-fno-signed-zeros` +
  reassociation. **Never** enable it blindly on a safety-relevant path; know
  that it breaks NaN checks. `-ffp-contract=fast` (the default!) lets the
  compiler fuse `a*b+c` into an FMA, changing results.

## 6. Cheatsheet: compiling for this course

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -g file.cpp -o out
clang++ -std=c++20 -Weverything -Wno-c++98-compat file.cpp   # firehose, but educational
g++ -std=c++20 -O2 -S -masm=intel file.cpp -o -              # look at the asm
```
Know these three tools by name: **Compiler Explorer (godbolt.org)** for asm,
**quick-bench.com** for microbenchmarks, **cppreference.com** as the reference
(not the standard, but close enough).

---

## Traps checklist (say these out loud)

1. Signed overflow = UB; unsigned = wraps.
2. `v.size() - 1` on an empty container = huge number.
3. Mixed sign comparison converts the signed side to unsigned — use
   `std::cmp_less`.
4. Everything smaller than `int` gets promoted to `int` before arithmetic.
5. `auto` decays; `auto&&`/`const auto&` do not.
6. `auto b = vec_of_bool[i]` gives you a proxy, not a `bool`.
7. Float equality needs a mixed absolute+relative tolerance.
8. FP addition is not associative → parallel reductions are non-deterministic.
9. `NaN != NaN`; use `std::isnan`; `-ffast-math` deletes your NaN checks.
10. Float→int cast out of range is UB.
