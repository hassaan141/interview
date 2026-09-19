# 01 — Mock interview questions

Cover the answers. Say each one **out loud** — the interview is verbal.

## A. Rapid fire (15 seconds each)

1. What is the difference in behavior between signed and unsigned integer overflow?
2. Why can the compiler delete `if (x + 1 < x)` for `int x`?
3. What is `std::size_t` and why must a loop index over a container use it (or `std::ssize`)?
4. What does integral promotion do, and at what type does it stop?
5. `int x = -1; unsigned u = 1;` — is `x < u` true? Why? What is the C++20 fix?
6. Name three things `auto` strips off a type.
7. Why is `auto b = vec_of_bool[0];` not a `bool`?
8. What is `sizeof(char)` guaranteed to be? What about `sizeof(int)`?
9. Which is faster in a loop, `i++` or `++i`, and for which types does it matter?
10. How many bits of mantissa does a `float` have, and roughly how many decimal digits?
11. Give three properties that are true of `NaN`.
12. Is floating-point addition associative? What is the practical consequence?
13. What is machine epsilon, and why is `if (fabs(a-b) < 1e-9)` wrong for large values?
14. What is a denormal number and why do people care about them for performance?
15. What does `-ffast-math` actually enable, and name one thing it silently breaks.
16. What is the difference between `std::int32_t` and `std::int_fast32_t`?
17. Which operator has higher precedence: `&` or `==`? Why does that matter?
18. What's the difference between `std::strong_ordering` and `std::partial_ordering`?
19. What does "trivially copyable" buy you, concretely?
20. What does "standard layout" buy you, concretely?

## B. Code reading — what happens and why

**B1.**
```cpp
std::vector<int> v;
for (int i = 0; i < v.size() - 1; ++i) std::cout << v[i];
```

**B2.**
```cpp
float sum = 0.0f;
for (int i = 0; i < 10'000'000; ++i) sum += 0.1f;
std::cout << sum;   // what is printed, roughly, and why not 1000000?
```

**B3.**
```cpp
std::uint16_t a = 0xFFFF;
std::uint16_t b = 0xFFFF;
auto c = a * b;         // what is the type of c? what is its value?
```

**B4.**
```cpp
double d = 1e20;
int i = static_cast<int>(d);   // what is i?
```

**B5.**
```cpp
int mask = 0b1010;
if (mask & 0b0010 == 0b0010) std::cout << "set";
```

**B6.**
```cpp
const std::map<std::string, int> counts = load();
for (std::pair<std::string, int> kv : counts) { /* ... */ }
```

**B7.**
```cpp
bool is_valid(double x) { return x == x; }   // compiled with -ffast-math
```

**B8.**
```cpp
template <typename T> void f(T);
int arr[10];
f(arr);       // what is T?
const int ci = 3;
f(ci);        // what is T?
```

## C. Whiteboard / discussion

**C1.** Write a float comparison function you would actually ship in a control
loop. Justify every parameter.

**C2.** Write a running mean and variance over a stream of `double` telemetry
samples, without storing the samples, that does not suffer catastrophic
cancellation. State the recurrence.

**C3.** You are given a 32-bit timestamp counter that wraps. Write
`elapsed(uint32_t now, uint32_t then)` that is correct across a single wrap.
Why does this work?

**C4.** A perception pipeline reduction gives bit-different results when you run
it on 8 threads vs. 1. Product asks "is it a bug?" Explain, and describe two
ways to make it deterministic and what each costs.

**C5.** You need a struct that goes into shared memory read by a C process on a
different compiler. List every property it must have and how you would enforce
them at compile time.

---
---

# Answers

**A1.** Signed overflow is **undefined behavior** — the optimizer assumes it
cannot happen and may remove code that depends on it. Unsigned overflow is
well-defined: it wraps modulo 2^N.

**A2.** Because `x + 1 < x` can only be true if signed overflow occurred, and UB
"cannot happen", so the compiler folds the expression to `false`. Use
`__builtin_add_overflow` / `std::add_sat` (C++26) / promote to a wider type.

**A3.** `std::size_t` is an unsigned integer type large enough to hold the size
of any object; it is what `sizeof` and `.size()` return. Using `int` invites
sign-conversion warnings and truncation on >2^31 element buffers; mixing it with
signed indices triggers the usual-arithmetic-conversion trap. `std::ssize()`
(C++20) returns a signed size when you want signed arithmetic.

**A4.** Any integral type with rank lower than `int` (`bool`, `char`, `short`,
bitfields, enums with small underlying types) is converted to `int` (or
`unsigned int` if `int` cannot represent all its values) before the operation.
It stops at `int`/`unsigned int`.

**A5.** `x < u` is **false**. `int` and `unsigned int` have the same rank, so the
signed operand converts to unsigned: `-1` becomes `4294967295`. Fix:
`std::cmp_less(x, u)` from `<utility>`.

**A6.** Top-level `const`/`volatile`, references, and array/function decay to
pointer. (`auto&`, `const auto&`, `auto&&` and `decltype(auto)` preserve them.)

**A7.** `std::vector<bool>` is a bit-packed specialization; `operator[]` returns
a proxy class `std::vector<bool>::reference` that writes back to the bit. `auto`
deduces the proxy, and the proxy can dangle if the vector dies.

**A8.** `sizeof(char) == 1` always (and `CHAR_BIT >= 8`). `sizeof(int)` is only
guaranteed `>= sizeof(short)` and `<= sizeof(long)`; it is 4 on essentially every
mainstream platform but that is not a guarantee.

**A9.** `++i` is never slower. For built-in scalars the compiler emits identical
code. For class types (iterators, big integers) postfix must copy the object
before incrementing, so `it++` costs a copy.

**A10.** 23 stored mantissa bits, 24 effective (implicit leading 1) →
~7.2 decimal digits. `double`: 52 stored / 53 effective → ~15.9 digits.

**A11.** (1) `NaN != NaN` is true and all ordered comparisons with NaN are false;
(2) NaN propagates through arithmetic; (3) there are quiet and signaling NaNs,
and many bit patterns are NaN (exponent all ones, non-zero mantissa). Detect
with `std::isnan`.

**A12.** No. Each operation rounds, so grouping changes the result.
Consequence: any reordering — vectorization, multithreaded reduction,
`-ffast-math` reassociation — changes the numeric output. Determinism requires
fixing the reduction order.

**A13.** Machine epsilon is the difference between 1.0 and the next
representable value (`2.22e-16` for `double`) — it is *relative*, not absolute.
The absolute gap between neighbors scales with magnitude, so at `1e16` the gap
is ~2.0 and an absolute `1e-9` tolerance is unreachable. Use a mixed
absolute + relative test.

**A14.** A subnormal has a zero exponent field and no implicit leading 1, giving
gradual underflow near zero at reduced precision. Some CPUs handle them in
microcode, costing 10-100x per operation; that is why FTZ/DAZ modes and
`-ffast-math` flush them to zero.

**A15.** `-ffast-math` = `-fno-math-errno -funsafe-math-optimizations
-ffinite-math-only -fno-rounding-math -fno-signaling-nans -fcx-limited-range
-fexcess-precision=fast`, and it sets FTZ/DAZ. It lets the compiler reassociate,
assume no NaN/Inf, and ignore signed zero. It silently breaks `std::isnan` /
`x != x` checks (they fold to false) and makes results non-reproducible.

**A16.** `int32_t` is **exactly** 32 bits (and only exists if the platform has
such a type). `int_fast32_t` is **at least** 32 bits, chosen for speed — it may
be 64 bits. Use `int32_t` for layout/ABI, `int_fast32_t` for loop counters where
you only need a minimum width.

**A17.** `==` binds tighter than `&`. So `mask & 0b0010 == 0b0010` parses as
`mask & (0b0010 == 0b0010)` = `mask & 1`. Always parenthesize bitwise ops.

**A18.** `strong_ordering` means equivalent values are *substitutable* —
indistinguishable in any observable way. `weak_ordering` means equivalent but
distinguishable (case-insensitive string compare). `partial_ordering` allows
*unordered* pairs — required for floating point because of NaN.

**A19.** You may `memcpy` the object bytes and get a valid copy — so you can put
it in shared memory, DMA it, serialize it, or store it in a lock-free ring
buffer, and `std::vector<T>` can relocate it with `memmove` instead of
element-wise moves.

**A20.** The member layout and offsets are C-compatible and unspecified only
between *different* access-control groups (of which there must be at most one
non-static group). That means `offsetof` is well-defined and a C translation
unit or another compiler can agree on the layout — the requirement for a
cross-language IPC struct or a hardware register map.

---

**B1.** `v.size()` is `size_t`. `0 - 1` wraps to `SIZE_MAX`, `i` is `int` so it
is converted to unsigned for the comparison and the condition is effectively
always true → reads far out of bounds → ASan heap-buffer-overflow. Also
`-Wsign-compare` warns.

**B2.** Around **1.09e6**, not 1e6, and it stops growing. Two effects:
(1) `0.1f` is not 0.1, and (2) once `sum` is large enough that `0.1f` is below
half a ULP of `sum`, `sum += 0.1f` rounds back to `sum` and the loop stagnates
(around 2^21 for `float`). Use `double`, or Kahan summation, or
`n * 0.1`.

**B3.** `c` is `int` (both operands promote to `int`). `0xFFFF * 0xFFFF =
4294836225`, which **overflows** `int` → **undefined behavior**. Fix:
`static_cast<std::uint32_t>(a) * b`.

**B4.** `1e20` does not fit in `int` → **undefined behavior** (in practice
`INT_MIN` on x86 via `cvttsd2si`, but do not rely on it). Clamp first, or use
`std::lround` into a wide enough type after a range check.

**B5.** Prints `"set"` only if bit 0 of `mask` is set. `mask & (0b0010 ==
0b0010)` = `mask & 1` = `0b1010 & 1` = 0, so it prints nothing. Precedence bug.

**B6.** `counts`'s `value_type` is `std::pair<const std::string, int>`, which is
not `std::pair<std::string, int>` — so every iteration constructs a **new pair
and copies the string**. Write `for (const auto& [key, n] : counts)`.

**B7.** Under `-ffast-math` (`-ffinite-math-only`) the compiler assumes no NaNs,
so `x == x` folds to `true` and `is_valid` always returns true. The NaN check is
deleted. Use `std::isnan` and do not build safety checks with `-ffast-math`, or
isolate the check in a TU compiled without it.

**B8.** For `f(arr)`: by-value template parameters decay, so `T = int*`. For
`f(ci)`: top-level `const` is dropped for by-value, so `T = int`. With
`void f(T&)` you would get `int(&)[10]` and `const int&`.

---

**C1.**
```cpp
// atol guards the region around zero where relative error is meaningless;
// rtol handles large magnitudes where the ULP gap exceeds any fixed atol.
// Pick atol from the physical resolution of the quantity (e.g. 1e-6 m for a
// position), rtol from the accumulated rounding budget (~ n_ops * eps).
constexpr bool near(double a, double b, double atol, double rtol) {
    if (std::isnan(a) || std::isnan(b)) return false;      // NaN is never "near"
    if (a == b) return true;                               // handles both infinities
    const double diff = std::fabs(a - b);
    return diff <= atol || diff <= rtol * std::max(std::fabs(a), std::fabs(b));
}
```
Say the NaN and infinity cases out loud — that is the part candidates forget.

**C2.** Welford:
```
n    <- n + 1
d    <- x - mean
mean <- mean + d / n
M2   <- M2 + d * (x - mean)     // note: the UPDATED mean
var  <- M2 / n            (population)   or   M2 / (n-1)  (sample)
```
It never forms `E[x^2] - E[x]^2`, so there is no large-minus-large subtraction.
Mention Neumaier/Kahan summation as the alternative for plain sums, and that
Welford has a parallel (Chan) merge form if you need to combine per-thread
partials.

**C3.**
```cpp
std::uint32_t elapsed(std::uint32_t now, std::uint32_t then) {
    return now - then;      // unsigned arithmetic is modulo 2^32
}
```
Unsigned subtraction wraps by definition, so the difference is correct as long
as less than 2^32 ticks elapsed. Doing this with `int32_t` would be signed
overflow → UB. This is exactly how monotonic hardware counters are compared in
embedded code, and it is a very common interview question.

**C4.** Not a bug — FP addition is not associative, so a different reduction
tree gives a different (equally valid) rounding. Making it deterministic:
(a) **fixed-shape reduction** — deterministic pairwise/tree reduction with a
fixed chunk size independent of thread count: cheap, small constant overhead;
(b) **higher-precision or compensated accumulation** — Kahan/Neumaier, or
accumulate `float` data into `double`, or integer/fixed-point accumulation:
costs 2-4x on the reduction but is order-independent for integers and
near-so for compensated sums. Also mention disabling `-ffp-contract=fast` and
`-ffast-math`, and that the real requirement is usually *reproducibility for
bit-exact replay of logged drives*, which is why this matters at Tesla.

**C5.** Requirements: `std::is_standard_layout_v<T>` (C-compatible offsets),
`std::is_trivially_copyable_v<T>` (memcpy-able), fixed-width members from
`<cstdint>` (no `int`/`long`/`size_t`/`bool` — `bool` size is
implementation-defined), explicit padding made visible, a pinned `alignas`, no
virtuals, no references, no pointers (addresses are not valid in another
process), a version field, and a defined byte order. Enforce:
```cpp
struct alignas(8) Frame {
    std::uint32_t version;
    std::uint32_t seq;
    std::uint64_t timestamp_ns;
    std::int32_t  x_mm, y_mm;
};
static_assert(std::is_standard_layout_v<Frame>);
static_assert(std::is_trivially_copyable_v<Frame>);
static_assert(sizeof(Frame) == 24);          // pin the size
static_assert(offsetof(Frame, timestamp_ns) == 8);
static_assert(alignof(Frame) == 8);
```
Bonus points: mention `-Wpadded` to find implicit padding, and that padding
bytes are indeterminate so you must `memset` before writing to shared memory if
you ever compare or hash the raw bytes.
