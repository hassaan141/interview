# 02 — Entities, Enums, `struct`/`union`/Bitfields, Control Flow, Namespaces, Attributes

Course chapter: **6 (Basic Concepts IV)**

Smaller section, but it contains three things embedded/autonomy code uses
constantly: `enum class`, bitfields, and `union`/`std::variant` for message
decoding.

---

## 1. Declaration vs. definition

- A **declaration** introduces a name and its type.
- A **definition** is a declaration that also fully specifies the entity
  (allocates storage, provides a body, completes a class).

```cpp
extern int g;              // declaration
int g = 3;                 // definition
void f();                  // declaration
void f() {}                // definition
struct S;                  // declaration (incomplete type)
struct S { int a; };       // definition
```

You may declare as many times as you like; you may define **once** per program
(One Definition Rule — see section 08). An incomplete type is enough for
pointers, references, and function declarations — that is what makes forward
declarations a build-time optimization.

## 2. Enumerators — always `enum class`

```cpp
enum Color { Red, Green };                // unscoped: Red leaks into the scope,
                                          // implicitly converts to int
enum class State : std::uint8_t {         // scoped: State::Idle, fixed 1-byte
    Idle = 0, Driving = 1, Fault = 255
};
```

`enum class` gives you: no implicit conversion to integer, no name leakage, and
a **guaranteed underlying type** (so it is safe in a packed wire struct). Convert
explicitly:

```cpp
auto raw = static_cast<std::uint8_t>(State::Fault);
auto st  = static_cast<State>(raw);           // note: NOT validated!
// C++23: std::to_underlying(State::Fault)
```

Two things to say in an interview:
- Casting an out-of-range integer into an enum with a fixed underlying type is
  fine (value is just that integer); with an *unfixed* underlying type, a value
  outside the enum's bit-range is **UB**. So always fix the underlying type when
  parsing external data, and validate.
- `switch` over an `enum class` with no `default` gives you
  `-Wswitch`/`-Wswitch-enum` coverage warnings — a free exhaustiveness check.
  That is a real argument to make when someone asks about "best practices".

## 3. `struct`, bitfields, `union`

```cpp
struct Pose { double x, y, theta; };          // aggregate: brace-initializable
Pose p{1.0, 2.0, 0.0};
Pose q{.x = 1.0, .theta = 0.5};               // C++20 designated initializers
                                              // (must be in declaration order)
```

### Bitfields
```cpp
struct CanStatus {
    std::uint32_t id       : 11;   // CAN 2.0A identifier
    std::uint32_t rtr      : 1;
    std::uint32_t reserved : 4;
    std::uint32_t dlc      : 4;
    std::uint32_t          : 0;    // unnamed zero-width: force next field to a
    std::uint32_t crc      : 15;   // new allocation unit
};
```
What is **implementation-defined**: allocation order within the unit (LSB-first
on gcc/clang on x86/ARM little-endian, but not portable), whether a field may
straddle an allocation unit, the alignment of the unit. So bitfields are
excellent for compact in-memory state and **dangerous for wire formats across
toolchains**. The portable alternative is explicit shifts and masks:

```cpp
constexpr std::uint32_t can_id(std::uint32_t frame) { return frame & 0x7FFu; }
constexpr std::uint32_t can_dlc(std::uint32_t frame) { return (frame >> 16) & 0xFu; }
```
Also: you cannot take the address of a bitfield, cannot bind a non-const
reference to one, and two threads writing different bitfields **in the same
allocation unit** is a data race (the compiler does a read-modify-write of the
whole unit). That last one is a great answer to a concurrency question.

### `union` and the active member
```cpp
union Raw { float f; std::uint32_t u; };
Raw r; r.f = 1.0f;
auto bits = r.u;     // reading the INACTIVE member is UB in C++
                     // (it is legal in C — this is a classic trick question)
```
Correct type punning, in order of preference:
```cpp
auto bits = std::bit_cast<std::uint32_t>(1.0f);   // C++20, constexpr, no UB
std::memcpy(&bits, &f, sizeof bits);              // pre-C++20, optimizes to a mov
// reinterpret_cast<std::uint32_t&>(f)            // strict-aliasing violation, UB
```
For a tagged variant, use `std::variant` (section 12) — it tracks the active
member and throws `std::bad_variant_access` instead of being UB.

## 4. Control flow — the modern forms

```cpp
// Init-statement in if / switch (C++17): scope the variable to the branch.
if (auto it = map.find(key); it != map.end()) { use(it->second); }
switch (auto code = poll(); code) { /* ... */ }

// Range-based for, and its lifetime trap:
for (const auto& x : make_vector()) {}        // OK: temporary lifetime extended
for (const auto& x : make_vector().field) {}  // DANGLING before C++23!
for (auto&& x : make_vector().field) {}       // still dangling pre-C++23
// C++20 fix:
for (auto v = make_vector(); const auto& x : v.field) {}   // init-statement

// C++20: structured bindings + init-statement
for (const auto& [k, v] : m) {}
```

`switch` rules to state: the controlling expression must be integral or
enumeration (or convertible); `case` labels must be constant expressions;
fallthrough is implicit — annotate it with `[[fallthrough]];`; you cannot jump
over an initialization into a `case` (use braces to scope).

`goto` — one legitimate use: breaking out of nested loops in C-style error
cleanup. In C++ use a lambda + `return`, or a named flag, or (C++ has no labeled
break) factor the loops into a function.

## 5. Namespaces

```cpp
namespace tesla::autonomy {            // C++17 nested namespace definition
    void tick();
}
namespace ta = tesla::autonomy;        // alias
using tesla::autonomy::tick;           // using-DECLARATION: one name. Fine.
using namespace tesla::autonomy;       // using-DIRECTIVE: never at file scope in
                                       // a header. ADL surprises + ODR hazards.
::global_fn();                         // explicit global namespace

namespace {                            // anonymous/unnamed namespace:
    int helper_state;                  // internal linkage, unique per TU.
    void helper();                     // THE correct way to make a TU-local
}                                      // helper in C++ (better than `static`,
                                       // because it also works for types)

inline namespace v2 { struct Msg {}; } // inline namespace: names are visible in
                                       // the enclosing namespace -> ABI versioning
```

**Argument-dependent lookup (ADL)** is the thing to be able to explain: for
`f(x)`, the compiler also searches the namespaces of `x`'s type. That is why
`swap(a, b)` finds your custom `swap`, and why the idiom is
`using std::swap; swap(a, b);` rather than `std::swap(a, b)`. It is also why
`operator<<` for your type must live in *your* namespace.

## 6. Attributes worth knowing

```cpp
[[nodiscard]] Status init();               // warn if the return value is ignored
[[nodiscard("check the error code")]]      // C++20: with a reason
[[maybe_unused]] int debug_only;           // suppress the unused warning
[[deprecated("use tick_v2")]] void tick();
[[noreturn]] void panic();                 // never returns -> better codegen/analysis
[[fallthrough]];                           // intentional switch fallthrough
[[likely]] / [[unlikely]]                  // C++20 branch hints (see section 14)
[[assume(x > 0)]]                          // C++23: lets the optimizer assume
[[no_unique_address]]                      // C++20: allow empty members to take 0 bytes
```

`[[nodiscard]]` on every function returning a status or an owning handle is the
single cheapest correctness win in a foundations library. Have that opinion
ready.

---

## Traps checklist

1. Unscoped `enum` implicitly converts to `int` and leaks names — use
   `enum class` with a fixed underlying type.
2. Casting an arbitrary integer to an enum with an *unfixed* underlying type is UB.
3. Bitfield layout is implementation-defined → don't use it for wire formats.
4. Concurrent writes to different bitfields in the same allocation unit = data race.
5. Reading an inactive union member is UB in C++ (legal in C) — use
   `std::bit_cast`/`memcpy`/`std::variant`.
6. `for (auto& x : f().member)` dangles before C++23 — use the init-statement.
7. `using namespace` in a header is a defect, not a style choice.
8. Anonymous namespace, not `static`, for TU-local entities in C++.
9. `switch` without `default` over an `enum class` gives you free exhaustiveness
   warnings — that is a feature, keep it.
