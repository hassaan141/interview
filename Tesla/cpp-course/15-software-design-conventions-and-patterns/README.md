# 15 — Software Design, Code Conventions, SOLID, C++ Idioms and Patterns

Course chapters: **15 (Code Conventions I), 16 (Code Conventions II), 26 (Software
Design I), 27 (Software Design II)**

The posting says you will *"evangelize best software practices"*. That means you need
opinions you can defend, not a list of rules you memorized.

---

## 1. Project organization

```
autonomy_core/
├── CMakeLists.txt
├── README.md  LICENSE  .clang-format  .clang-tidy  .gitignore
├── include/autonomy_core/       ← the ONLY public surface
│   ├── frame.hpp
│   └── detector.hpp
├── src/                         ← implementation + private headers
│   ├── detector.cpp
│   └── internal/ring_buffer.hpp
├── tests/                       ← unit + integration
├── benchmarks/
├── examples/
├── docs/
├── cmake/                       ← Find*.cmake, Config.cmake.in
└── third_party/  (or a package manager)
```
Key ideas: a **single public include directory** whose path is namespaced
(`#include <autonomy_core/frame.hpp>`), private headers **not** installed, and CMake
`PUBLIC`/`PRIVATE` enforcing it (section 13). `.hpp`/`.cpp` (or `.h`/`.cc`) — pick one
and never mix. One class per header, named after the class.

## 2. Naming and formatting — pick a style and stop arguing

Popular styles: **Google**, **LLVM**, **Core Guidelines**, **AUTOSAR/MISRA C++**
(automotive — worth knowing the name), and the project's own. The choice matters far
less than consistency, and consistency should be **enforced by a tool**
(`.clang-format` + `.clang-tidy` `readability-identifier-naming`), not by reviewers.

A defensible default:
```cpp
namespace tesla::autonomy {                 // lower_snake namespaces

class FrameDecoder {                        // PascalCase types
public:
    static constexpr int kMaxFrames = 64;   // kPascalCase constants
    using value_type = Frame;               // lower_snake for STL-compatible names

    bool decode_next(std::span<const std::byte> bytes);   // lower_snake functions
    [[nodiscard]] std::size_t frames_decoded() const noexcept;

private:
    std::size_t count_{0};                  // trailing underscore for members
    static int instances_;
};

enum class DecodeStatus : std::uint8_t { Ok, Truncated, BadCrc };   // PascalCase values
constexpr double kGravity = 9.80665;
#define TESLA_AUTONOMY_LOG(...)             // MACROS ARE SCREAMING AND PREFIXED
}  // namespace tesla::autonomy
```
Formatting rules worth having an opinion about: 80 or 100 columns (pick one),
`int* p` vs `int *p` (pick one — attach to the type, since the type is `int*`), braces
on the same line for functions, always brace the body of an `if` (the `goto fail` bug),
one declaration per line, east-`const` vs west-`const` (pick one; `const int` is more
common, `int const` reads right-to-left more consistently).

Avoid, with reasons: Hungarian notation (the type system already knows),
abbreviations that are not universal (`idx` yes, `mngr` no), `l`/`O`/`I` as names,
names that lie, and a leading underscore followed by a capital (reserved to the
implementation).

## 3. Documentation

```cpp
/// Decodes the next frame from `bytes`.
///
/// @param bytes  A complete or partial frame. Not retained.
/// @return       The decoded frame, or an error describing why it failed.
/// @pre          `bytes.size() <= kMaxFrameSize`
/// @post         On success, `frames_decoded()` has increased by one.
/// @note         Never allocates; safe to call from a real-time context.
/// @complexity   O(bytes.size())
[[nodiscard]] std::expected<Frame, DecodeStatus>
decode(std::span<const std::byte> bytes) noexcept;
```
Document **why**, preconditions, postconditions, ownership, thread safety, complexity,
and units. Do **not** document what the code already says (`// increment i`). A comment
that repeats the code becomes a lie on the first edit. Doxygen (or a `docs/` markdown
tree) generates the reference; the comments in the header are the contract.

Say the units in the *name* rather than a comment: `timeout_ms`, `distance_m`,
`velocity_mps`. Better still, use a strong type or `std::chrono`.

## 4. Design principles

- **Separation of concerns** — one module, one reason to change.
- **Low coupling, high cohesion** — few dependencies between modules, strongly related
  things inside one.
- **Encapsulation and information hiding** — the interface hides the representation; you
  can change the representation without changing callers. (PIMPL takes this to the ABI.)
- **Design by contract** — preconditions (the caller's duty), postconditions (yours),
  invariants (always true). Document them, `assert` them, and decide who checks.
- **Problem decomposition** — split until each piece is testable alone.
- **Code reuse** — but composition over inheritance, and do not create a dependency just
  to avoid twenty lines of duplication (the "DRY taken too far" failure mode).
- **Least astonishment** — a `size()` that is O(n) or a `get` that mutates is a defect.

**Class invariant** — the property every public method preserves; `private` data is how
you enforce it.

**Software entropy / technical debt** — code degrades unless maintained; debt is a
deliberate trade you must track, not an accident you hide.

## 5. SOLID, with C++-specific commentary

| | Principle | In C++ |
| --- | --- | --- |
| **S** | Single responsibility — one reason to change | a `Frame` that parses, validates, logs, and renders is four classes |
| **O** | Open for extension, closed for modification | add a new `Detector` implementation without editing the framework; a `dynamic_cast` chain or a `switch` over types is a violation |
| **L** | Liskov substitution — a subtype must honour the base's contract | do not strengthen preconditions or weaken postconditions in an override; the classic violation is `Square : Rectangle` |
| **I** | Interface segregation — clients should not depend on methods they do not use | prefer several small pure-virtual interfaces to one fat one; concepts do this at compile time |
| **D** | Dependency inversion — depend on abstractions, not concretions | take an interface (or a template parameter) rather than constructing your dependency; this is what makes code testable |

Say this about SOLID in C++: it was written for a virtual-dispatch world, and in C++
the **compile-time** versions are often better — a concept instead of an interface,
a template parameter instead of dependency injection, `std::variant` instead of a
hierarchy. Use virtual dispatch where the extension point must be open at runtime or
cross an ABI (section 06). That is a mature answer.

Also worth naming: **YAGNI** (do not build the abstraction you do not need yet — the
most commonly violated principle in C++ codebases), **law of Demeter**
(`a.b().c().d()` couples you to three types), and **prefer free functions** — a function
that can be written with the public interface should not be a member (it keeps the class
minimal and the function reusable).

## 6. Class design specifics

- **Member vs. free function**: make it a member only if it needs private access or must
  be virtual. Everything else is a free function in the same namespace (found by ADL).
  This is Scott Meyers's rule and it produces smaller, more testable classes.
- **Namespace-scope functions vs. static members**: prefer a namespace function; a class
  used only as a namespace is a code smell (and you cannot extend it).
- **Owning objects vs. views**: an owner manages lifetime (`std::vector`,
  `unique_ptr`); a view refers (`span`, `string_view`, a reference). Make the
  distinction visible in every signature, and never store a view whose owner you do not
  control.
- **Value vs. reference semantics**: value semantics (copyable, comparable, no aliasing)
  is the default and the reason C++ is safe to reason about; reference semantics
  (polymorphic, identity-based, shared) is for entities with a lifetime and identity.
  Choose deliberately, per type, and say which one you chose.
- **Global variables**: avoid. They defeat testing, create the static-init-order fiasco
  (section 08), are hidden inputs, and are unsynchronized. If you must, use a
  function-local static behind an accessor, and prefer passing a context object.

## 7. C++ idioms

**Rule of zero / three / five** — section 05.

**RAII** — section 05. The most important idiom in the language.

**PIMPL (pointer to implementation)** — hide the representation behind an opaque
pointer:
```cpp
// detector.hpp — no implementation details, no heavy includes
class Detector {
public:
    Detector();
    ~Detector();                            // must be OUT OF LINE (incomplete type)
    Detector(Detector&&) noexcept;
    Detector& operator=(Detector&&) noexcept;
    void detect(std::span<const float> in);
private:
    struct Impl;                            // declared only
    std::unique_ptr<Impl> impl_;
};
```
Buys: **ABI stability** (the header's layout never changes), **build-time reduction**
(the header includes nothing), and a real encapsulation boundary. Costs: one heap
allocation, one indirection per call, no inlining into callers. The destructor, move
operations, and any defaulted special member must be defined in the `.cpp`, where
`Impl` is complete — forgetting that is the classic compile error.

**CRTP (curiously recurring template pattern)** — static polymorphism:
```cpp
template <typename Derived>
class Shape {
public:
    double area() const { return static_cast<const Derived&>(*this).area_impl(); }
    void print() const { /* shared code, no vtable */ }
};
class Circle : public Shape<Circle> {
    friend class Shape<Circle>;
    double r_{};
    double area_impl() const { return 3.14159 * r_ * r_; }
};
```
Zero-overhead "virtual" dispatch resolved at compile time — no vptr, fully inlinable.
Costs: no common base type (you cannot put them in one container without a variant),
code bloat, worse error messages. C++23's **explicit object parameter** (`this auto&&
self`) and C++20 concepts cover many CRTP uses more cleanly.

**Singleton** — the pattern to be able to *criticize*:
```cpp
class Config {
public:
    static Config& instance() { static Config c; return c; }   // thread-safe since C++11
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
private:
    Config() = default;
};
```
Why to avoid: it is global mutable state with a hidden dependency, it makes unit testing
hard (you cannot substitute it), destruction order is unspecified relative to other
statics, and it invites unsynchronized access from multiple threads. What to do instead:
construct it once in `main` and **pass it in** (dependency injection) — the object still
exists once, but the dependency is explicit and substitutable.

Other patterns worth naming and knowing when to use:
- **Factory** — hide construction and return an interface (`std::unique_ptr<Base>`);
  the only way to "fail a constructor" without exceptions.
- **Strategy** — inject the algorithm; in C++ often a template parameter or a
  `function_ref`, not a virtual.
- **Observer** — but beware the lifetime problem (section 04) and calling callbacks under
  a lock (section 11).
- **Visitor / double dispatch** — or just `std::variant` + `std::visit` for a closed set.
- **Type erasure** (`std::function`, `std::any`, `std::shared_ptr<void>`, or a
  hand-rolled vtable) — value semantics over an open set of types; the technique behind
  `AnyDetector`-style APIs.
- **NVI (non-virtual interface)** — public non-virtual methods that do the invariant
  checks and call protected virtual hooks. This is how you enforce a contract on
  subclasses.
- **Template virtual functions** — the workaround (a virtual taking an erased argument)
  because a function template cannot be virtual.
- **Builder** — for a type with many optional parameters; combine with ref-qualifiers
  (section 09) so the final `build() &&` moves out.

---

## The opinions to have ready

1. **Rule of zero.** Own resources through members; declare none of the six.
2. **`const` by default, `explicit` by default, `[[nodiscard]]` on anything fallible.**
3. **Make interfaces hard to misuse**: strong types for units, `enum class`, no
   `bool` parameters (use an enum), no out-parameters, deleted lossy overloads.
4. **Value semantics unless you need identity.**
5. **Views in parameters, owners in members.** Never the reverse.
6. **Compile-time polymorphism inside, virtual at the boundary.**
7. **No global mutable state.** Pass a context.
8. **Enforce style with tooling, not review comments.** Save review for design.
9. **YAGNI** beats a speculative abstraction every time.
10. **The unit of design is the interface**, and the test for a good one is whether you
    can change the implementation without touching a caller.

---

## Traps checklist

1. A PIMPL destructor and move operations must be defined out of line.
2. CRTP gives you no common base type — you cannot store them heterogeneously.
3. `Square : Rectangle` is the canonical Liskov violation.
4. A `dynamic_cast` chain or a type `switch` is an open/closed violation.
5. Singleton is global state: unit-testable only by accident.
6. A comment that restates the code becomes a lie after the first edit.
7. `bool` parameters at a call site are unreadable — use an `enum class`.
8. Do not document units in a comment; put them in the name or the type.
9. A view stored as a member is a dangling bug waiting to happen.
10. SOLID assumes virtual dispatch; in C++ prefer the compile-time equivalents.
