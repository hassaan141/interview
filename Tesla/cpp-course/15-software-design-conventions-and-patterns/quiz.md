# 15 — Mock interview questions

"Evangelize best software practices" means they will ask you to *defend* an opinion.

## A. Rapid fire

1. What belongs in `include/` vs `src/`? How do you enforce it?
2. Why namespace the public include path (`#include <autonomy_core/frame.hpp>`)?
3. What is a class invariant? How do you enforce one?
4. State the five SOLID principles in one line each.
5. Give the C++ **compile-time** equivalent of each of the five.
6. What is the canonical Liskov violation, and why is it one?
7. What is an open/closed violation you can spot in a code review instantly?
8. When should a function be a member? When a free function?
9. Value semantics vs. reference semantics — how do you choose?
10. Why avoid global mutable state? Give four distinct reasons.
11. What does PIMPL buy you? What does it cost? What must be defined out of line?
12. What does CRTP buy you? What is the cost you cannot design around?
13. Why is Singleton criticized, and what do you do instead?
14. What is type erasure? Name three standard-library examples.
15. What is NVI and what problem does it solve?
16. Why can a function template not be virtual? What is the workaround?
17. Why is a `bool` parameter a design defect?
18. Where do units belong: a comment, the name, or the type?
19. What is YAGNI and what does violating it look like in C++?
20. What is the law of Demeter, and what does violating it cost you?
21. How should coding style be enforced?
22. What does a good doc comment contain that the code does not?

## B. Critique the design

**B1.**
```cpp
class FrameManager {
public:
    void loadFromDisk(const std::string& path);
    void parse();
    void validate();
    void render(Canvas&);
    void uploadToServer(const std::string& url);
    void log(const std::string& msg);
    static FrameManager& getInstance();
};
```

**B2.**
```cpp
class Rectangle {
public:
    virtual void set_width(double w)  { w_ = w; }
    virtual void set_height(double h) { h_ = h; }
    double area() const { return w_ * h_; }
protected: double w_{}, h_{};
};
class Square : public Rectangle {
    void set_width(double s)  override { w_ = h_ = s; }
    void set_height(double s) override { w_ = h_ = s; }
};
```

**B3.**
```cpp
void process(Shape* s) {
    if (auto* c = dynamic_cast<Circle*>(s))        { /* ... */ }
    else if (auto* r = dynamic_cast<Rect*>(s))     { /* ... */ }
    else if (auto* t = dynamic_cast<Tri*>(s))      { /* ... */ }
}
```

**B4.**
```cpp
// detector.hpp
#include <Eigen/Dense>
#include <opencv2/opencv.hpp>
#include <vector>
class Detector {
    Eigen::MatrixXd weights_;
    cv::Mat scratch_;
    std::vector<Detection> results_;
public:
    void detect(const cv::Mat& frame);
};
```

**B5.**
```cpp
class Logger {
public:
    void log(int severity, bool to_file, bool to_console, bool flush, const char* msg);
};
logger.log(2, true, false, true, "started");
```

**B6.**
```cpp
struct Waypoint { double x, y, timeout, speed; };   // meters? feet? ms? s? m/s? mph?
void go_to(const Waypoint& w, int timeout);
```

**B7.**
```cpp
class Pipeline {
    Database db_{"prod-host:5432"};     // constructed in the member init list
    HttpClient http_{"https://api"};
public:
    Result run(const Input&);
};
```

**B8.**
```cpp
class Buffer {
public:
    std::string_view data() const { return view_; }   // view_ points into a member
private:
    std::vector<char> storage_;
    std::string_view  view_{storage_.data(), storage_.size()};
};
```

## C. Whiteboard

**C1.** Design the public interface of `autonomy_core`'s frame-transport layer: a
producer publishes frames, N consumers read them, no allocation in steady state, and it
must be usable from both a real-time thread and an offline replay tool. State every
design decision.

**C2.** A colleague proposes making every class in the codebase inherit from a common
`Object` base with `virtual std::string to_string()` and `virtual Object* clone()`.
Respond.

**C3.** You are asked to write the team's C++ style guide in one page. What is on it,
and what do you deliberately leave out?

**C4.** Refactor this for testability and explain each change:
```cpp
class Planner {
public:
    Trajectory plan() {
        auto now = std::chrono::system_clock::now();
        auto cfg = Config::instance();
        auto map = MapLoader::load("/opt/maps/current.bin");
        auto obs = g_perception_output;          // global
        if (cfg.debug) std::cout << "planning\n";
        return solve(map, obs, now, cfg.horizon);
    }
};
```

---
---

# Answers

**A1.** `include/<project>/` holds only the headers a consumer needs; `src/` holds
implementation and private headers. Enforce it with CMake:
`target_include_directories(lib PUBLIC include PRIVATE src)` — then a consumer physically
*cannot* `#include "internal/ring_buffer.hpp"`, because that path is not on their include
path. Convention alone does not hold; the build system does.

**A2.** So the include is unambiguous and self-documenting, and so two dependencies can
both have a `frame.hpp` without colliding. It also makes the installed layout match the
source layout, and it prevents a consumer's `-I` from accidentally shadowing your header.

**A3.** A property that is true in every observable state of the object (`size_ <=
capacity_`, "`fd_` is -1 or a valid descriptor"). The constructor establishes it (throwing
or failing if it cannot), every public method preserves it, the destructor may assume it.
Enforce with `private` data plus a narrow interface, `assert`/`CHECK` at method
boundaries, and NVI (public non-virtual method checks, protected virtual hook) when
subclasses are involved.

**A4.** **S**ingle responsibility: one reason to change. **O**pen/closed: extendable
without modification. **L**iskov: a subtype must be usable wherever the base is, honouring
its contract. **I**nterface segregation: clients should not depend on methods they do not
use. **D**ependency inversion: depend on abstractions, and inject them.

**A5.** S: a small class or a free function (unchanged). O: a **template parameter** or a
**concept** — a new type satisfies the concept without editing anything. L: a **concept's
requirements** are the contract, checked at compile time. I: several **small concepts**
instead of one fat interface (`std::ranges` is built this way). D: take the dependency as
a **template parameter** (static injection) rather than a virtual interface — no vtable,
fully inlinable, and still substitutable in tests. Use the runtime versions where the
extension point must be open at runtime or cross an ABI.

**A6.** `Square : Rectangle` with independent `set_width`/`set_height`. Code written
against `Rectangle` may reasonably assume "set the width, the height is unchanged"; a
`Square` breaks that postcondition, so a function `void grow(Rectangle& r) { r.set_width(
r.width()*2); assert(r.area() == old*2); }` fails for a `Square`. The lesson: inheritance
must model **behavioural** substitutability, not "is a kind of" in English.

**A7.** A chain of `dynamic_cast`s, or a `switch` over a type tag, in a function that
must be edited every time a new type is added. Also: an `enum` of subclasses, and
`if (typeid(*p) == typeid(X))`. The fix is a virtual function, a visitor, or a closed
`std::variant`.

**A8.** A member only if it needs **private access** or must be **virtual** (or is one of
the operators the language requires to be a member: `=`, `[]`, `()`, `->`, conversions).
Everything else is a **free function in the same namespace**, found by ADL: it keeps the
class's interface minimal (less to maintain, less to break), makes the function reusable
across types, and gives symmetric conversion behaviour for binary operators.

**A9.** **Value semantics** (copyable, comparable, no aliasing, no identity) is the
default: it is local to reason about, thread-friendly, and works with the STL. Use
**reference semantics** when the object has an **identity** and a lifetime that matters —
a hardware device, a connection, a node in a graph, a subscription — where copying would
be wrong. Choose per type, deliberately, and make it visible (a move-only type says
"unique"; a copyable one says "value").

**A10.** (1) It defeats **testing** — you cannot substitute it, and tests cannot run in
parallel or in isolation. (2) It is a **hidden input** — a function's behaviour depends on
state its signature does not mention. (3) It creates the **static
initialization/destruction order fiasco** (section 08). (4) It is **unsynchronized** —
any multithreaded access is a data race unless you audit every use. Bonus: it prevents
having two instances, which you eventually need (a second sensor, a replay harness).

**A11.** Buys: **ABI stability** (the header's layout never changes, so you can add
members without recompiling consumers), **build-time reduction** (the header includes
nothing heavy, so a change to the implementation recompiles one TU), and a genuine
encapsulation boundary. Costs: one heap allocation per object, one pointer indirection per
member access, **no inlining** into callers, and more boilerplate. Must be out of line:
the **destructor**, the **move constructor and move assignment**, and any defaulted
special member — because they need `Impl` to be complete, and the header only declares
it.

**A12.** Buys: polymorphic *code reuse* with **zero runtime overhead** — no vptr in the
object, dispatch resolved at compile time, fully inlinable, and usable in `constexpr`. The
cost you cannot design around: **there is no common base type**, so `Shape<Circle>` and
`Shape<Square>` are unrelated types and you cannot store them in one container or pass
them through one non-template function. (Workarounds: `std::variant` for a closed set,
type erasure for an open one.) Secondary costs: code bloat and poor error messages.
C++23's explicit object parameter (`this auto&& self`) replaces many CRTP uses.

**A13.** Because it is **global mutable state with a hidden dependency**: untestable
(cannot substitute a fake), unmockable, unsynchronized, impossible to have two of, and its
destruction order relative to other statics is unspecified. What to do instead: create
exactly one instance in `main` and **pass it in** (constructor injection, or a context
object). You keep "there is one of these" as a *policy* while the dependency stays
explicit and substitutable — see `Watchdog` in `examples.cpp`.

**A14.** Storing an arbitrary type behind a fixed interface while preserving **value
semantics** for the holder: internally a `Concept` base plus a `Model<T>` template, held
by `unique_ptr`, with a `clone()` for copying. Standard examples: `std::function`,
`std::any`, `std::shared_ptr<void>`'s deleter, `std::pmr::memory_resource`,
`std::ranges::any_view` (C++26). It gives you an open type set without inheritance leaking
into the user's types.

**A15.** **Non-Virtual Interface**: the public methods are non-virtual and do the
invariant checks, logging, locking, and bookkeeping; they call **private/protected virtual
hooks** that subclasses override. It solves "a subclass can forget the contract" — the
subclass cannot bypass the checks, because it does not override the public entry point.
`Stage::run`/`run_impl` in `examples.cpp`. It also means you can change the public
signature or add instrumentation without touching any subclass.

**A16.** Because a virtual call is dispatched through a **vtable slot chosen at compile
time**, and a template is an unbounded family of functions — the compiler would have to
create a slot per instantiation, but instantiations are discovered per translation unit,
so the vtable layout could differ between TUs (an ODR/ABI impossibility). Workarounds: a
non-template virtual taking an **erased** argument (`std::any`, a `std::variant`, a
`Visitor&`, or a `std::span<std::byte>`), or CRTP, or a virtual that takes a type-erased
callback.

**A17.** Because at the **call site** it carries no information: `log(2, true, false,
true, msg)` is unreadable and transposing two arguments compiles silently. Use an
`enum class` per flag (`Blocking::Yes`), a small options struct with designated
initializers (`connect({.blocking = true})`), or separate named functions. This is a
"make interfaces hard to misuse" rule, and it is cheap.

**A18.** In the **type** if you can (a strong type, or `std::chrono::milliseconds`), in
the **name** otherwise (`timeout_ms`, `distance_m`). Never only in a comment — comments do
not propagate to call sites and do not fail the build. A strong type turns a unit bug into
a compile error (see `Quantity` in `examples.cpp`) for zero runtime cost.

**A19.** "You Aren't Gonna Need It": do not build an abstraction for a requirement you do
not have. In C++ it looks like a template parameter with one instantiation, an interface
with one implementation, a factory that constructs one type, a plugin system with no
plugins, a policy class nobody varies, and `AbstractFrameProviderFactoryBase`. It costs
build time, readability, and the ability to refactor — because the abstraction has frozen
a design you had not yet validated. The counter-rule: make it concrete, and extract the
abstraction the *second* time you need it.

**A20.** "Only talk to your immediate friends": `a.b().c().d()` couples you to the types of
`b`, `c`, **and** `d`, so a change to any of them breaks you, and you cannot test `a`
without building all of them. It also usually signals a missing method on `a`. The
exceptions are fluent builders and range pipelines, where the chaining *is* the interface.

**A21.** With **tooling, in CI**: a committed `.clang-format` (and `clang-format --dry-run
--Werror` as a build gate) plus `.clang-tidy` with `readability-identifier-naming`. A
style rule enforced by review comments wastes reviewer attention on the cheapest possible
issue and is applied inconsistently; automate it and spend review on design. Also: format
the whole codebase in one commit so `git blame` has exactly one formatting commit to skip
(`.git-blame-ignore-revs`).

**A22.** The things the code cannot state: **why** (the rationale and the alternative you
rejected), **preconditions and postconditions**, **ownership** (does it retain the
pointer?), **thread safety** (safe to call concurrently? from a real-time thread?),
**complexity**, **units**, **error behaviour** (throws? allocates? blocks?), and the
invariant it maintains. What it should *not* contain is a restatement of the code, which
becomes a lie on the first edit.

---

**B1.** Four problems. (1) **Single responsibility**: this class does disk I/O, parsing,
validation, rendering, networking, and logging — six reasons to change, and it cannot be
unit tested without a disk, a canvas, and a network. Split into `FrameLoader`,
`FrameParser`, `FrameValidator`, `FrameRenderer`, `FrameUploader`, and use the existing
logger. (2) **Singleton** (`getInstance`) — global mutable state, see A13. (3) The name
**`Manager`** is a smell: it names no responsibility. (4) Inconsistent naming
(`loadFromDisk` camelCase vs the rest) and `log` duplicating a cross-cutting concern.

**B2.** The canonical **Liskov violation** (A6). Concretely: a function that takes
`Rectangle&`, sets width to 4 and height to 5, and asserts `area() == 20` fails for a
`Square`. Fixes: do not inherit — make `Square` a separate type (or a factory function
returning a `Rectangle` with equal sides); or make the shapes **immutable** (no setters),
in which case the substitution is fine; or model it as a single `Rectangle` with an
`is_square()` query. The general lesson: inheritance models behavioural contracts, not
English taxonomy.

**B3.** Open/closed violation plus three RTTI lookups per call. Every new shape edits this
function, an unhandled type silently does nothing, and `dynamic_cast` is neither cheap nor
inlinable. Fixes: a **virtual method** on `Shape` (if the operation belongs to the shape);
a **visitor** with `virtual void accept(Visitor&)` (if the operation belongs elsewhere and
the type set is closed-ish); or **`std::variant` + `std::visit`** for a closed set, which
also gives compile-time exhaustiveness.

**B4.** The public header leaks its entire implementation: every consumer pays for
`<Eigen/Dense>` and `<opencv2/opencv.hpp>` (tens of thousands of lines each), every
consumer's build breaks when Eigen's version changes, the class's **layout is part of the
ABI** so adding a member forces a recompile of the world, and `cv::Mat` in the interface
forces OpenCV on callers who do not want it. Fixes: **PIMPL** the members; take
`std::span<const std::uint8_t>` plus width/height/stride rather than `cv::Mat` in the
public signature; forward declare `Detection`; move both heavy includes to the `.cpp`.

**B5.** Three `bool` parameters — unreadable at the call site and trivially transposable
(A17), plus `int severity` instead of an `enum class`, plus `const char*` instead of
`std::string_view`. Rewrite: `void log(Severity, std::string_view msg, LogSinks sinks =
LogSinks::Default)` with `enum class` flags, or an options struct with designated
initializers.

**B6.** No units anywhere: `x`/`y` could be metres or pixels, `timeout` could be seconds
or milliseconds, `speed` m/s or mph — and there are now **two** timeouts with no stated
relationship. Fix: strong types (`Meters x, y; Seconds timeout; Mps speed;`) or
`std::chrono::milliseconds`, and put the unit in the name where a strong type is
overkill. Then the transposed-argument and wrong-unit bugs become compile errors.

**B7.** The class **constructs its own dependencies**, hard-codes a production hostname,
and therefore cannot be unit tested at all: every test hits a real database and a real
API, and there is no way to inject a failure. Fix: inject them
(`Pipeline(IDatabase&, IHttpClient&)`, or as template parameters), pass configuration in
rather than baking it in, and provide fakes in tests. This is dependency inversion, and it
is the single most common testability defect.

**B8.** The member initializer captures `storage_.data()` and `storage_.size()` **before
anything is in the vector** (so it is an empty view), and worse, the view **dangles the
moment `storage_` reallocates or the object is copied/moved** — the copy's `view_` still
points into the original's buffer. Never store a view alongside its owner. Fix: compute
the view on demand (`std::string_view data() const { return {storage_.data(),
storage_.size()}; }`), which is correct and free.

---

**C1.** Public interface for frame transport:
```cpp
namespace tesla::autonomy {

// A frame is a VIEW plus metadata; the transport owns the storage.
struct FrameView {
    std::uint64_t seq{};
    std::uint64_t capture_time_ns{};
    std::uint32_t width{}, height{}, stride{};
    std::span<const std::byte> pixels;             // non-owning
};

// A lease: RAII, so a slot is released on every exit path (section 05).
class FrameLease {
public:
    FrameLease(FrameLease&&) noexcept;
    FrameLease& operator=(FrameLease&&) noexcept;
    ~FrameLease();                                  // returns the slot to the pool
    FrameLease(const FrameLease&) = delete;         // a lease is unique
    [[nodiscard]] FrameView view() const noexcept;
    explicit operator bool() const noexcept;
private:
    friend class FrameTransport;
    FrameLease(FrameTransport*, std::uint32_t slot) noexcept;
    FrameTransport* owner_{};
    std::uint32_t   slot_{};
};

class FrameTransport {
public:
    // Fixed capacity, allocated ONCE at construction. No allocation afterwards.
    static std::expected<FrameTransport, TransportError>
    create(std::uint32_t slots, std::uint32_t bytes_per_frame);

    // Producer side: acquire a writable slot, fill it, publish it.
    [[nodiscard]] std::optional<WriteLease> acquire_for_write() noexcept;  // nullopt = full
    void publish(WriteLease&&) noexcept;

    // Consumer side: each consumer has its own cursor, so N consumers do not
    // interfere and a slow consumer cannot block the producer.
    [[nodiscard]] Subscription subscribe();
    [[nodiscard]] std::optional<FrameLease> try_next(Subscription&) noexcept;

    // Observability is part of the interface, not an afterthought.
    struct Stats { std::uint64_t published, dropped, slots_in_use; };
    [[nodiscard]] Stats stats() const noexcept;
};
}  // namespace tesla::autonomy
```
Decisions, each with a reason:
- **Views + leases, not values**: the transport owns a preallocated pool, so there is
  **no allocation and no copying** in steady state. A `FrameLease` is RAII so a slot
  cannot be stranded by an early return or an exception. `FrameView` is trivially
  copyable and cheap to pass.
- **`create()` factory returning `expected`, not a throwing constructor**: the real-time
  consumer may be built `-fno-exceptions`, and a constructor's only failure channel is an
  exception (section 10).
- **Everything on the hot path is `noexcept` and non-allocating**, and says so.
  `try_next` returns `optional` rather than blocking — a real-time consumer must never
  block. A separate blocking `wait_next(timeout)` can exist for the offline tool.
- **Per-consumer cursors** so one slow consumer cannot stall the producer, and an explicit
  **overload policy** (`dropped` in `Stats`): when the ring is full the producer
  overwrites the oldest and increments a counter. Dropping must be visible, not silent.
- **Fixed capacity chosen from Little's law** (section 14): slots ≥ rate × worst-case
  consumer latency, with headroom.
- **Same API for real-time and replay**: the replay tool uses the same transport with a
  different clock and a blocking `wait_next`, so the production code path is the tested
  code path. That is the property that makes an evaluation pipeline meaningful (section
  13).
- **Value/reference split stated explicitly**: `FrameTransport` and `FrameLease` have
  identity (move-only), `FrameView` and `Stats` are values.
- **No `std::function`, no virtual, no `std::string` on the hot path.** Polymorphism, if
  needed, goes at the boundary (section 06).

**C2.** Respond with the concrete costs, not "that's Java":
1. **It breaks value semantics and triviality.** A virtual destructor gives every object a
   **vptr**, so nothing is trivially copyable or standard layout any more — no `memcpy` into
   shared memory, no DMA, no lock-free ring buffer of them, and `std::vector<T>` can no
   longer relocate with `memmove` (sections 06, 12). For a `Vec3` or a `Frame` header this
   is a measurable regression in the hot path and an outright blocker for IPC.
2. **It costs size and cache.** 8 bytes per object, plus a vtable per class. A 12-byte
   `Vec3` becomes 24 with padding — halving how many fit in a cache line.
3. **`clone()` forces heap allocation** and reference semantics onto types that should be
   values, and `Object*` return types reintroduce manual ownership.
4. **It is an interface-segregation violation**: `to_string()` is meaningless for most
   types, so you get a forest of `return "<unimplemented>"` overrides.
5. **It does not actually enable anything.** What do you want? Generic printing → a free
   `operator<<`/`std::formatter` specialization, or a `Printable` **concept**, both with
   zero cost and no base class. Heterogeneous containers → `std::variant` (closed) or type
   erasure (open), both with value semantics. Reflection → a `constexpr` field-list
   pattern or a code generator.
Then close constructively: "Let's identify the actual requirement. If it is 'I want to log
any object', I will add a `Formattable` concept and a `std::formatter` pattern this
afternoon, with no change to any existing type." Offering the cheaper alternative is what
makes the objection land.

**C3.** One page, in priority order — the things that prevent bugs, not the things that
settle taste:
**On the page:**
1. Formatting is `.clang-format`; naming is `.clang-tidy`. **Not negotiable, not
   reviewed by humans.** (One line, and it removes 80% of style discussion.)
2. `-Wall -Wextra -Wpedantic -Werror` in CI. No warnings in main.
3. **Rule of zero.** Own resources through members; declare none of the six unless you
   must, and then declare all five.
4. **RAII for every resource.** No `new`/`delete`, no manual `lock()`/`unlock()`, no
   `goto cleanup`.
5. `const` by default. `explicit` on single-argument constructors and conversion
   operators. `[[nodiscard]]` on anything fallible or owning. `noexcept` on moves and
   `swap`.
6. **Views in parameters (`span`, `string_view`), owners in members.** Never store a
   view.
7. **No global mutable state.** Pass a context; inject dependencies.
8. **Make interfaces hard to misuse**: strong types or units in names, `enum class` not
   `bool`, no out-parameters, `= delete` the lossy overloads.
9. **Public headers are minimal**: forward declare, no heavy includes, PIMPL what needs
   ABI stability. Private headers live in `src/`.
10. **Tests**: every bug fix starts with a failing test. ASan+UBSan in CI. No `sleep` in
    a test.
11. **Real-time rules** (this codebase specifically): no allocation, no locks, no
    exceptions, no unbounded loops on the hot path — and say so in the doc comment.
12. Document **why**, plus preconditions, ownership, thread safety, and units.

**Deliberately left out**, and I would say why: tabs vs. spaces, brace placement, column
limit, east-const vs. west-const, `#pragma once` vs. include guards, Hungarian-adjacent
prefixes, banning `auto`, banning templates, banning exceptions *globally* (it is a
per-layer decision), and any rule that a tool cannot check — because an unenforceable rule
is a decoration, and a guide full of taste rules trains people to ignore the guide.

**C4.**
```cpp
// Before: five hidden dependencies (wall clock, singleton config, the filesystem, a
// global, and stdout). Untestable, unparallelizable, non-deterministic.
class Planner {
public:
    // Inject everything the function actually depends on.
    Planner(const IClock& clock, const Config& cfg, const Map& map, ILogger& log)
        : clock_{clock}, cfg_{cfg}, map_{map}, log_{log} {}

    // The per-call input is a PARAMETER, not a global.
    [[nodiscard]] Trajectory plan(const PerceptionOutput& obs) const {
        const auto now = clock_.now();                     // injected clock
        log_.debug("planning");                            // injected sink, level-filtered
        return solve(map_, obs, now, cfg_.horizon);        // pure, given its inputs
    }
private:
    const IClock& clock_;
    const Config& cfg_;
    const Map&    map_;
    ILogger&      log_;
};
```
Change by change:
- **`system_clock::now()` → an injected `IClock`**: the test controls time, so the test is
  deterministic and needs no `sleep` (section 13). Also `system_clock` was the wrong clock
  for elapsed time anyway (section 12).
- **`Config::instance()` → a `const Config&` member**: removes the global, makes the
  dependency visible in the constructor, and lets two planners run with different configs
  (which you will need for A/B evaluation).
- **`MapLoader::load(path)` → an injected `const Map&`**: the planner is no longer coupled
  to the filesystem or a hard-coded path, loading happens once instead of per call (a
  latency bug as well as a testability one), and a test can supply a two-node map.
- **`g_perception_output` → a `plan(obs)` parameter**: the function's inputs are now in its
  signature. This is the single biggest change: `plan` becomes a **pure function of its
  arguments**, so it can be unit tested, fuzzed, replayed, and run in parallel.
- **`std::cout` → an injected `ILogger`**: tests capture it, the level filter is
  centralized, and the hot path does not do synchronous I/O.
- **`const` on `plan`** now that there is no hidden mutation, and `[[nodiscard]]` because
  discarding a trajectory is a bug.
Then state the payoff: the test becomes
`Planner{FakeClock{}, cfg, tiny_map, NullLogger{}}.plan(fixture_obs)` with no I/O, no
globals, and a deterministic result — which is exactly what lets it run in the evaluation
pipeline.
