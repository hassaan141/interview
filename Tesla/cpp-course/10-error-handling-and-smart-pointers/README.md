# 10 — Undefined Behavior, Error Handling, `noexcept`, Smart Pointers

Course chapter: **22 (Advanced Topics II), part 1**

Ownership and error handling are the two things a foundations library gets asked to
standardize. Have an opinion, and be able to defend it on latency grounds.

---

## 1. Undefined, unspecified, implementation-defined

| Term | Meaning | Example |
| --- | --- | --- |
| **undefined behavior** | no requirements at all; the compiler may assume it never happens | signed overflow, OOB access, use-after-free, data race, reading an uninitialized value, null deref, strict-aliasing violation, `INT_MIN/-1`, shift >= width, infinite loop with no side effects |
| **unspecified behavior** | one of several valid behaviours, implementation need not document it | order of evaluation of function arguments, order of `a() + b()` |
| **implementation-defined** | unspecified, but must be documented | `sizeof(int)`, signedness of `char`, bitfield layout, `>>` on negatives (well-defined since C++20) |
| **ill-formed, no diagnostic required (IFNDR)** | invalid program, compiler need not tell you | ODR violations, a `constexpr` function no invocation can be constant-evaluated |

Why UB is not "just a crash": the optimizer **assumes UB never occurs** and deletes
code accordingly. `if (p == nullptr) return;` after `p->x` can be removed, because
the dereference already proved `p != nullptr`. That is why "it worked in debug" is
meaningless.

Detection, in the order you should reach for them:
```bash
-fsanitize=address,undefined   # ASan + UBSan: OOB, UAF, leaks, signed overflow, ...
-fsanitize=thread               # data races (cannot combine with ASan)
-fsanitize=memory               # uninitialized reads (clang; needs an MSan libc++)
valgrind --leak-check=full      # slower, no recompile needed
-D_GLIBCXX_ASSERTIONS           # cheap libstdc++ bounds checks (also -D_GLIBCXX_DEBUG)
-fstack-protector-strong -D_FORTIFY_SOURCE=3
```

## 2. Error-handling strategies — the decision table

| Mechanism | Cost on the happy path | Cost on error | When |
| --- | --- | --- | --- |
| **preconditions + assert/contract** | zero in release | UB or abort | programmer errors: a null argument, an out-of-range index. **Not** for runtime conditions |
| **return code / `enum class Status`** | a branch | a branch | C ABI boundaries, `-fno-exceptions` builds, hot loops |
| **`std::optional<T>`** | a branch + a bool | a branch | "absent" is normal and needs no explanation (a lookup miss) |
| **`std::expected<T, E>`** (C++23) | a branch + a tag | a branch | recoverable errors where the *reason* matters, in a hot path |
| **exceptions** | **zero** (no branch, no check) | expensive: unwind tables, RTTI match, ~1-10 µs | genuinely exceptional, non-local, deep call stacks; constructors |
| **`std::terminate` / fail fast** | zero | process dies | invariant violated, no safe recovery |

The key fact to state about exceptions: **they are zero-cost on the non-throwing
path** (the "Itanium zero-cost EH" model — no runtime check, the unwind information
lives in a separate table). What they cost is *binary size* (unwind tables, typically
5-15%), the loss of some optimizations, and a **non-deterministic, unbounded latency
when they throw** — which is exactly why hard-real-time and embedded code compiles
with `-fno-exceptions`.

So for this role, the answer is: "exceptions for construction failure and for truly
exceptional conditions in non-real-time code; `std::expected` or a status enum in the
real-time path, because I need a bounded worst case; and `assert`/contracts for
programmer errors, which should never be recoverable."

```cpp
// exceptions
class SensorError : public std::runtime_error {
    int code_;
public:
    SensorError(int code, const std::string& what)
        : std::runtime_error{what}, code_{code} {}
    int code() const noexcept { return code_; }
};
try { risky(); }
catch (const SensorError& e) { /* most derived FIRST */ }
catch (const std::exception& e) { std::cerr << e.what(); }
catch (...) { }                       // catch-all; rethrow with a bare `throw;`

// std::expected (C++23) -- the modern recoverable-error type
enum class ParseError { TooShort, BadChecksum, UnknownId };
std::expected<Frame, ParseError> parse(std::span<const std::byte> bytes) {
    if (bytes.size() < 8)   return std::unexpected(ParseError::TooShort);
    if (!crc_ok(bytes))     return std::unexpected(ParseError::BadChecksum);
    return Frame{/* ... */};
}
// monadic composition, no branching noise:
auto result = parse(bytes)
                  .transform([](Frame f) { return undistort(f); })
                  .and_then(detect)
                  .transform_error(to_string);
```
Standard exception hierarchy worth knowing: `std::exception` →
`std::logic_error` (`invalid_argument`, `domain_error`, `length_error`,
`out_of_range`) and `std::runtime_error` (`range_error`, `overflow_error`,
`underflow_error`, `system_error`), plus `std::bad_alloc`, `std::bad_cast`,
`std::bad_optional_access`, `std::bad_variant_access`.

**Rules:** throw by value, catch by `const&`. Never throw from a destructor. Order
`catch` clauses most-derived first. Always derive from `std::exception`. Prefer
`std::throw_with_nested` for context. Never use exceptions for control flow.

### `noexcept`
```cpp
void f() noexcept;                       // promises not to throw; if it does ->
                                         // std::terminate (no unwinding required)
void g() noexcept(sizeof(T) < 64);       // conditional
static_assert(noexcept(f()));            // the OPERATOR: is this expression noexcept?
```
Put `noexcept` on: move constructors and move assignment (mandatory in practice —
see section 09), `swap`, destructors (implicit), `size()`/`empty()`-style observers,
and anything a container queries. Do **not** sprinkle it on everything: it is part
of your interface, and removing it later is an API break, and inside a `noexcept`
function the compiler must emit a terminate handler for any call that could throw.

Destructors are implicitly `noexcept`. Throwing from one during unwinding calls
`std::terminate`.

## 3. Smart pointers

### `std::unique_ptr<T, D>` — exclusive ownership, zero overhead
```cpp
auto p = std::make_unique<Widget>(args...);     // prefer this
auto arr = std::make_unique<Widget[]>(10);      // array form calls delete[]
std::unique_ptr<FILE, decltype(&std::fclose)> f{std::fopen("x","r"), &std::fclose};

// custom deleter as a stateless type keeps sizeof == one pointer (EBO):
struct FdCloser { void operator()(int* fd) const noexcept { ::close(*fd); } };
static_assert(sizeof(std::unique_ptr<Widget>) == sizeof(Widget*));
```
Moveable, not copyable. `release()` gives up ownership, `reset()` replaces,
`get()` observes. It is the default answer to "who owns this?".

### `std::shared_ptr<T>` — shared ownership, reference counted
```cpp
auto s = std::make_shared<Widget>(args...);     // ONE allocation (object+control block)
std::shared_ptr<Widget> s2{new Widget};         // TWO allocations. Avoid.
std::weak_ptr<Widget> w = s;                    // non-owning observer
if (auto locked = w.lock()) { locked->use(); }  // atomically promote, or get null
```
Layout: the `shared_ptr` itself is **two pointers** (object + control block). The
control block holds a **strong count**, a **weak count**, the deleter, and the
allocator. Counts are updated with **atomic** RMW operations — that is the cost:
an atomic increment on every copy, an atomic decrement (plus a branch) on every
destruction, and cross-core cache-line contention if many threads copy the same
`shared_ptr`.

What is and is not thread safe: the **control block** is thread safe (you may copy
and destroy `shared_ptr`s to the same object from many threads). The **pointee** is
not, and neither is the same `shared_ptr` *object* being written by two threads —
for that you need `std::atomic<std::shared_ptr<T>>` (C++20).

`std::weak_ptr` breaks **cycles**: two objects holding `shared_ptr`s to each other
never reach zero and leak. Parent owns child with `shared_ptr`, child points back
with `weak_ptr`.

`std::enable_shared_from_this<T>` gives a member `shared_from_this()` so an object
can hand out a `shared_ptr` to itself — required for async callbacks that must keep
the object alive. Calling it when no `shared_ptr` owns the object throws
`std::bad_weak_ptr`.

### The decision order (say it exactly like this)
1. **A value member or a local** — no pointer at all. Default.
2. **`std::unique_ptr`** — one owner, polymorphism, or a stable address.
3. **`std::shared_ptr`** — genuinely shared, uncertain lifetime, async callbacks.
4. **Raw pointer / reference / `std::span` / `std::string_view`** — **non-owning
   observation only**, never ownership. A raw `T*` in a modern C++ interface means
   "optional, not owned".

`std::make_unique`/`make_shared` benefits: no raw `new`, exception safety in a
multi-argument call, one allocation for `make_shared`. `make_shared` downside: the
object's memory is only freed when the last **weak** reference dies, since the
control block and object share one allocation — a problem for large objects with
long-lived `weak_ptr`s.

---

## Traps checklist

1. UB lets the optimizer delete your checks — "it worked in debug" means nothing.
2. Exceptions are zero-cost when not thrown; the cost is binary size and unbounded
   throw latency. That is the real-time argument, not "exceptions are slow".
3. Never throw from a destructor; destructors are implicitly `noexcept`.
4. `catch` most-derived first; catch by `const&`; throw by value.
5. `noexcept` is part of your API — adding it later is fine, removing it is a break.
6. `shared_ptr` copies are atomic RMWs: pass `const shared_ptr&` or a raw
   pointer/reference to functions that only observe.
7. `shared_ptr` cycles leak — break with `weak_ptr`.
8. `std::shared_ptr<T>{new T}` costs two allocations; `make_shared` costs one.
9. `make_shared` keeps the object's storage alive as long as any `weak_ptr` exists.
10. A raw pointer in a modern interface means "non-owning and optional".
11. `unique_ptr` with a stateless deleter is exactly one pointer wide (EBO).
12. Do not allocate in a real-time path at all — smart pointers do not fix that.
