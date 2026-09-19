# 02 — Mock interview questions

## A. Rapid fire

1. Difference between a declaration and a definition. How many of each are allowed?
2. What can you do with an incomplete type? Why does that matter for build times?
3. Three concrete advantages of `enum class` over plain `enum`.
4. When is `static_cast<MyEnum>(some_int)` undefined behavior?
5. Name three things about bitfield layout that are implementation-defined.
6. Can two threads safely write two different bitfields of the same struct?
7. Is reading the inactive member of a `union` legal? In C? In C++?
8. What are the three correct ways to reinterpret a `float`'s bits as a `uint32_t`?
9. What does `[[nodiscard]]` do, and where would you put it in a library?
10. What is the difference between a using-declaration and a using-directive?
11. Why is `using namespace std;` in a header a defect?
12. Explain ADL in one sentence. Give one thing that only works because of it.
13. Why is an anonymous namespace preferred over `static` for a TU-local helper?
14. What is an `inline namespace` for?
15. What does `[[noreturn]]` let the compiler do?
16. Why should a `switch` over an `enum class` usually *not* have a `default`?

## B. Code reading

**B1.**
```cpp
struct Holder { std::vector<int> data{1,2,3}; };
Holder make();
for (int x : make().data) std::cout << x;     // C++17. Bug?
```

**B2.**
```cpp
enum Flags { None = 0, Read = 1, Write = 2 };
Flags f = Read | Write;          // compiles?
```

**B3.**
```cpp
union U { float f; std::uint32_t u; };
bool is_negative(float x) { U u; u.f = x; return u.u >> 31; }
```

**B4.**
```cpp
struct Packed { std::uint8_t a : 4; std::uint8_t b : 4; };
// sent over a serial link to a different MCU compiled with a different toolchain
```

**B5.**
```cpp
switch (state) {
    case State::Idle: x = 1;
    case State::Driving: x = 2; break;
}
```

**B6.**
```cpp
// header.hpp
using namespace std;
struct array { int data[4]; };
```

**B7.**
```cpp
namespace lib { struct Pose{double x;}; }
void print(const lib::Pose&);
std::vector<lib::Pose> v;
std::sort(v.begin(), v.end());    // what's missing, and where must it live?
```

## C. Whiteboard

**C1.** Decode a 4-byte little-endian CAN-like frame into
`{id:11, rtr:1, dlc:4, payload_len}` two ways: with bitfields and with
shifts/masks. Which do you ship and why?

**C2.** Design a `State` enum for a vehicle mode with a `to_string`, a
`from_wire(uint8_t)` that validates, and compile-time exhaustiveness. No
`default` in any switch.

**C3.** You have a message type that can be one of 5 payloads arriving at 100 Hz
in a hot loop. Compare `union` + tag, `std::variant`, and inheritance +
`unique_ptr`. Pick one and defend it on latency grounds.

---
---

# Answers

**A1.** A declaration introduces a name and its type; a definition additionally
provides the entity (storage, function body, complete class). Unlimited
declarations; exactly one definition per program (ODR) — except entities that
are `inline`, templates, or class types, which may be defined once *per
translation unit* with identical token sequences.

**A2.** Declare pointers and references to it, declare (not define) functions
taking/returning it, and `typedef` it. You cannot size it, dereference it, or
have it as a member by value. That is why forward declaring instead of
`#include`-ing cuts build time and breaks include cycles.

**A3.** (1) Scoped — no name leakage; (2) no implicit conversion to integer, so
`if (color)` and `color + 1` fail to compile; (3) fixed underlying type
(`: std::uint8_t`) so size and range are guaranteed, which makes it safe in a
packed struct; (4) can be forward-declared.

**A4.** With an **unfixed** underlying type, if the integer is outside the range
representable by the enum's bit-field of enumerator values, the result is
undefined (C++17 onward: unspecified/UB depending on version). With a **fixed**
underlying type the conversion is well-defined — the value is just that integer,
even if no enumerator has it. So: fix the underlying type and validate
separately.

**A5.** Allocation order within an allocation unit (high-to-low or low-to-high
bits), whether a field may straddle an allocation unit boundary, the size and
alignment of the allocation unit, whether a plain `int` bitfield is signed, and
how padding is inserted.

**A6.** **No** — not if they share an allocation unit. Reading/writing a bitfield
is a read-modify-write of the whole unit, so the two writes race and can lose
updates. The memory model gives you per-*object* races; bitfields in the same
unit are one memory location. Separate them with an unnamed zero-width field
`std::uint32_t : 0;` or use separate `std::atomic` members.

**A7.** In **C** it is legal (C explicitly allows type punning through a union).
In **C++** it is **undefined behavior** — only the active member may be read.
Classic trick question.

**A8.** `std::bit_cast<std::uint32_t>(f)` (C++20, `constexpr`, requires same
size and trivially copyable), `std::memcpy` (optimizes to a single move),
`std::as_bytes`/`std::span<const std::byte>` for a byte view.
`reinterpret_cast<std::uint32_t&>(f)` is a strict-aliasing violation → UB.

**A9.** It makes discarding the return value a warning (`-Wunused-result`). Put
it on every function returning an error/status code, every factory returning an
owning handle, and every pure observer (`empty()`, `size()`) where calling and
ignoring is always a bug.

**A10.** A using-**declaration** (`using std::swap;`) brings in one specific name
and participates in overload resolution locally. A using-**directive**
(`using namespace std;`) makes every name in the namespace visible for lookup,
changing overload resolution everywhere after it.

**A11.** Every TU that includes the header inherits the directive transitively,
so a new name added to `std` (or the other namespace) in a future standard can
silently change overload resolution or become ambiguous in code that never asked
for it. `std::byte`, `std::size`, `std::data`, `std::filesystem::path` all broke
code like this.

**A12.** For an unqualified call `f(x)`, the compiler also searches the
namespaces (and base-class namespaces) associated with the types of the
arguments. Only because of ADL do `std::cout << my_type`, `swap(a, b)`,
`begin(c)`/`end(c)` in a range-for, and customization points like `operator==`
find your namespace-scope overloads.

**A13.** `static` gives internal linkage only to variables and functions —
it cannot be applied to a class or an alias. An anonymous namespace gives
internal linkage to *everything* inside it, including types, and a class defined
in an anonymous namespace gets a unique name per TU, which avoids ODR
violations between two TUs that both define a local helper class with the same
name.

**A14.** ABI/API versioning: names in an `inline namespace` are also visible in
the enclosing namespace, so `std::v2::vector` can be referred to as
`std::vector` while still mangling to a distinct symbol. That is how libstdc++
does `std::__cxx11::basic_string` for the dual-ABI transition.

**A15.** Prove that control never reaches the code after the call: no "missing
return" warning, no reload of caller-saved state, the tail can be pruned, and
the static analyzer stops tracking that path. Useful on `panic`, `abort`
wrappers, and `throw` helpers.

**A16.** Because with no `default`, `-Wswitch` (on by default) warns when you add
a new enumerator and forget a case. A `default` silences that and turns a
compile-time exhaustiveness check into a silent runtime fallthrough. Handle the
unexpected-value case *after* the switch instead.

---

**B1.** Bug: only `make().data` — the member — has its lifetime considered; the
`Holder` temporary is destroyed at the end of the full-expression that
initializes the range, so `data` dangles. (Fixed in C++23 by P2718R0, which
extends the lifetime of all temporaries in the range expression.) Portable fix:
`for (auto h = make(); int x : h.data)`.

**B2.** No, it does not compile as written: `Read | Write` promotes both to `int`
and yields `int`, which does not implicitly convert back to `Flags`. (With
unscoped enums the *reverse* direction works, which is the confusing part.)
Either `Flags f = static_cast<Flags>(Read | Write);`, or use `enum class` plus
explicit `operator|` overloads, or a dedicated bitflag type.

**B3.** UB: writes `u.f`, reads `u.u`. Also unnecessary —
`return std::signbit(x);`. If you really want the bit:
`return std::bit_cast<std::uint32_t>(x) >> 31;`.

**B4.** Do not ship it. Which nibble lands in the low 4 bits is
implementation-defined, so the two toolchains can disagree and the link silently
transposes `a` and `b`. Use explicit shifts/masks on a `std::uint8_t`, define the
byte order in the protocol doc, and add a `static_assert` on `sizeof`.

**B5.** Missing `break` after `case State::Idle`, so `Idle` falls through and
`x` ends up 2. Either add `break` or, if intentional, `[[fallthrough]];` —
`-Wimplicit-fallthrough` catches it.

**B6.** Two defects: the using-directive in a header (poisons every includer)
and a type named `array` that will become ambiguous with `std::array` for any
includer — `array<int,3> a;` now fails to compile. Put the type in your own
namespace and delete the directive.

**B7.** `std::sort` needs a strict weak ordering, i.e. `operator<` (or `<=>`) for
`lib::Pose`, and it must be declared **in namespace `lib`** so ADL finds it from
the comparison inside `std::sort`. Defining `bool operator<(const lib::Pose&,
const lib::Pose&)` in the global namespace also works here, but the
namespace-scope-next-to-the-type version is the rule to state (it is required for
templates and for any two-phase lookup). Cleanest: `auto operator<=>(const
Pose&) const = default;` inside `Pose`.

---

**C1.**
```cpp
// (a) bitfields: readable, but layout is toolchain-defined
struct FrameBits {
    std::uint32_t id : 11; std::uint32_t rtr : 1;
    std::uint32_t pad : 4; std::uint32_t dlc : 4;
};
// (b) shifts and masks: portable, explicit, constexpr-friendly
struct Frame { std::uint32_t id; bool rtr; std::uint32_t dlc; };
constexpr Frame decode(std::uint32_t w) {
    return Frame{ w & 0x7FFu, ((w >> 11) & 1u) != 0u, (w >> 16) & 0xFu };
}
static_assert(decode(0x0008'0123u).id == 0x123);
```
Ship (b). Bitfields are fine for *in-process* compactness where only your
compiler reads them; anything crossing a process, a chip, or a toolchain gets
explicit shifts, a `static_assert` on sizes, and a documented byte order. Also
mention: `std::byteswap` (C++23) / `__builtin_bswap32` for endianness, and that
you would write the decoder `constexpr` so you can unit-test it with
`static_assert`.

**C2.**
```cpp
enum class Mode : std::uint8_t { Park = 0, Drive = 1, Reverse = 2, Neutral = 3 };

constexpr std::string_view to_string(Mode m) {
    switch (m) {                             // no default -> -Wswitch guards us
        case Mode::Park:    return "Park";
        case Mode::Drive:   return "Drive";
        case Mode::Reverse: return "Reverse";
        case Mode::Neutral: return "Neutral";
    }
    return "<invalid>";                      // only for out-of-range casts
}
constexpr std::optional<Mode> from_wire(std::uint8_t raw) {
    switch (static_cast<Mode>(raw)) {        // safe: underlying type is fixed
        case Mode::Park: case Mode::Drive:
        case Mode::Reverse: case Mode::Neutral: return static_cast<Mode>(raw);
    }
    return std::nullopt;                     // reject unknown wire values
}
static_assert(from_wire(1) == Mode::Drive);
static_assert(!from_wire(9).has_value());
```
Points to make: fixed underlying type for the wire, validation at the boundary
(parse, don't validate later), `constexpr` so the tests are `static_assert`s, no
`default` so adding `Mode::Sport` breaks the build in exactly the two places
that must change.

**C3.**
- **`union` + tag**: smallest, no indirection, no allocation, but you hand-manage
  the active member and non-trivial members need manual ctor/dtor calls. Fastest
  and most dangerous.
- **`std::variant`**: same storage shape (max member size + tag, no heap), type
  safe, `std::visit` dispatches through a jump table or a chain of compares.
  Overhead is a tag byte and, for `std::visit`, usually an indirect jump —
  measurably cheaper than a virtual call because there is no pointer chase to a
  vtable and the payload is inline (cache-friendly).
- **Inheritance + `unique_ptr`**: one heap allocation *per message* plus a vtable
  indirection plus a cache miss on the pointer chase. At 100 Hz with 5 types this
  is the worst choice on latency and it fragments the heap.

Pick **`std::variant`** (or a hand-rolled tagged union behind a checked API if
profiling shows `visit` in the way): value semantics, no allocation, contiguous
storage so a `std::vector<Message>` stays cache-friendly, and compile-time
exhaustiveness via a visitor with no generic fallback. Mention that you would
`static_assert(sizeof(Message) <= 64)` to keep a message inside one cache line.
