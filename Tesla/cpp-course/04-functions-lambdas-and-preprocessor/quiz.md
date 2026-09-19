# 04 — Mock interview questions

## A. Rapid fire

1. When do you pass by value, by `const&`, by `&`, by `std::string_view`, by `std::span`?
2. Why does `std::span` beat a `(pointer, length)` pair? Why does it beat `std::vector<T>&`?
3. What is the lifetime rule you must respect with `string_view`/`span`?
4. Is the return type part of a function's signature?
5. Is `void f(int)` a different overload from `void f(const int)`? From `void f(int*)` vs `void f(const int*)`?
6. Rank these conversions in overload resolution: promotion, exact match, user-defined conversion, standard conversion, ellipsis.
7. Does a template or a non-template win when both match exactly?
8. Where may a default argument be declared, and when is it evaluated?
9. Give a use for `= delete` other than suppressing a copy.
10. What is a lambda, in terms of what the compiler actually generates?
11. Is a lambda's `operator()` `const` by default? How do you change that?
12. What does `[this]` capture? What does `[*this]` capture?
13. Which lambdas convert to a plain function pointer?
14. Name two costs of `std::function` that a template parameter does not have.
15. Why is `std::sort` faster than C's `qsort`?
16. What does `[x = std::move(p)]` give you that `[p]` does not?
17. Why must `#define SQUARE(x) x*x` be parenthesized, and why is that still not enough?
18. What do `#` and `##` do? Why can't the language replace them?
19. Why is a function-like macro wrapped in `do { ... } while (0)`?
20. What replaces `__FILE__`/`__LINE__` in C++20, and why does it work?
21. What does `__has_include` let you do?
22. Difference between `#pragma once` and an include guard.

## B. Find the bug

**B1.**
```cpp
std::function<void()> defer_work() {
    std::vector<int> data = load();
    return [&data] { process(data); };
}
```

**B2.**
```cpp
class Poller {
    std::vector<int> queue_;
public:
    void start(ThreadPool& pool) {
        pool.post([this] { drain(queue_); });
    }
};
```

**B3.**
```cpp
std::string_view name() { return std::string("temp") + "orary"; }
```

**B4.**
```cpp
// header.hpp
void connect(int port, int timeout = 100);
// impl.cpp
void connect(int port, int timeout = 100) { /* ... */ }
```

**B5.**
```cpp
auto counter = [n = 0] { return ++n; };
```

**B6.**
```cpp
void set_speed(double mps);
set_speed(30);          // was meant to be km/h
```

**B7.**
```cpp
#define MAX(a, b) ((a) > (b) ? (a) : (b))
int best = MAX(compute_expensive(), fallback());
```

**B8.**
```cpp
void log(const std::string& msg);
for (int i = 0; i < 1'000'000; ++i) log("tick");
```

**B9.**
```cpp
template <typename T> void print(const std::vector<T>& v);
void print(const std::vector<int>& v);
std::vector<int> v;
print(v);                 // which one, and is that what the author wanted?
```

**B10.**
```cpp
std::vector<std::function<int()>> handlers;
for (int i = 0; i < 3; ++i) handlers.push_back([&i] { return i; });
// what do the handlers return after the loop?
```

## C. Whiteboard

**C1.** Write a `ScopeGuard` that runs a callable on destruction, with a
`dismiss()`. Explain why the callable should be a template parameter and not a
`std::function`, and why the destructor must be `noexcept`.

**C2.** Write a `retry(n, callable)` that calls a callable up to `n` times until
it returns `true`, forwarding arbitrary arguments perfectly.

**C3.** You need a callback registry in a latency-critical loop: 8 subscribers,
called at 1 kHz. Compare `std::vector<std::function<void(const Frame&)>>`,
a `std::vector<void(*)(void*, const Frame&)>` + `void* ctx`, and a
compile-time `std::tuple` of callables. Pick one, with numbers.

**C4.** Convert this C-style API into a modern C++ interface, and justify each
change:
```cpp
int process(const float* data, int len, float* out, int out_cap, int* out_len);
```

---
---

# Answers

**A1.** By value: small trivially copyable types (≤ ~2 machine words) and sink
parameters you will `std::move` from. `const&`: large objects you only read.
`&`: objects you modify in place. `string_view`: read-only character data whose
buffer you do not own — avoids constructing a `std::string` per call.
`span`: any read-only or mutable contiguous range, replacing `(T*, size_t)`.

**A2.** vs. `(pointer, length)`: one argument instead of two that can disagree,
carries `size()`, gives you `begin()/end()`/range-for/bounds checks in debug
builds, and cannot be silently reordered. vs. `std::vector<T>&`: the caller is not
forced to own a `vector` — a `std::array`, a C array, a subrange, a memory-mapped
buffer, or a `static` table all convert.

**A3.** They are non-owning views: the referenced storage must outlive the view.
Safe as parameters and short-lived locals; a **data member** or a returned view is
a bug unless you can prove the owner outlives it. `string_view` is also not
guaranteed null-terminated, so never hand `.data()` to a C API.

**A4.** No. You cannot overload on return type alone. (You *can* get
return-type-dependent behaviour with a proxy class that has templated conversion
operators — a good bonus answer.)

**A5.** `f(int)` and `f(const int)` are the **same** function — top-level cv on a
by-value parameter is discarded from the signature. `f(int*)` and `f(const int*)`
are **different**, because there the `const` is not top-level (it qualifies the
pointee).

**A6.** Exact match > promotion > standard conversion > user-defined conversion >
ellipsis. Within a call, candidates are compared argument-by-argument and a
candidate wins only if it is not worse on any argument and better on at least one.

**A7.** The **non-template** wins. If both are templates, the more specialized
one wins (partial ordering).

**A8.** In one declaration per scope — conventionally the header declaration, and
never repeated in the definition (that is a redefinition error). They must be
trailing. They are evaluated **at each call site**, in the caller's context, which
is why `std::source_location::current()` as a default gives you the caller's
location.

**A9.** Banning implicit conversions at an API boundary:
`void set_timeout(std::chrono::milliseconds); void set_timeout(int) = delete;`
stops `set_timeout(500)` from compiling with ambiguous units. Also: deleting a
specific template specialization, deleting `operator new` to forbid heap
allocation of a type, and deleting rvalue overloads to prevent binding to
temporaries (`const char* c_str() && = delete`).

**A10.** A unique, unnamed non-union class type (closure type) with: a member per
capture, a (by default `const`) `operator()`, an implicit conversion to a function
pointer **only if captureless**, and deleted default ctor/assignment (until
C++20, where a captureless lambda is default-constructible and assignable).

**A11.** Yes, `const` by default. `mutable` makes it non-`const` so you can modify
by-copy captures. (`mutable` does **not** affect by-reference captures — those
were never `const` to begin with unless the captured object is.)

**A12.** `[this]` captures the **pointer** `this` by value; the closure can then
read and write the object's members, and it dangles if `*this` dies first.
`[*this]` (C++17) captures a **copy of the object** into the closure — safe to
outlive, but you are mutating a copy.

**A13.** Only captureless ones (including captureless generic lambdas, which give
you a family of function pointers). A capturing lambda has state, and a plain
function pointer has nowhere to put it.

**A14.** (1) Type erasure → an **indirect call** that cannot be inlined; (2) a
possible **heap allocation** if the callable exceeds the small-buffer size
(typically 16 bytes on libstdc++), plus the copy and the destructor call; (3) it
is not `noexcept`-friendly and it can throw `std::bad_function_call`; (4) it is
~32 bytes, so a vector of them is cache-unfriendly.

**A15.** `qsort` takes a **function pointer**, so the comparison is an indirect
call per comparison and cannot be inlined, and it operates on `void*` with a
runtime element size so it must `memcpy` elements. `std::sort` takes the
comparator as a **template parameter**, so the comparison inlines into the sort
body and the element type (and its move operations) are known — typically 2-3x
faster.

**A16.** It **moves** the pointer into the closure, transferring ownership.
`[p]` would try to *copy* a `unique_ptr`, which does not compile. Init-capture is
also how you create a closure member that does not correspond to any enclosing
variable.

**A17.** Without parens, `SQUARE(1+2)` textually expands to `1+2*1+2` = 5 —
operator precedence applies after substitution. Parenthesizing every parameter
and the whole body fixes that, but not **double evaluation**: `SQUARE(i++)`
expands the argument twice, so the side effect happens twice (and here, in one
expression, it is outright UB). A `constexpr` function evaluates its argument
exactly once.

**A18.** `#x` **stringizes** the argument's tokens; `a##b` **pastes** two tokens
into one identifier. Neither is expressible in the language because both operate
on source text before the language sees it — you cannot recover the spelling of an
expression (`#cond` in an assert message) or synthesize an identifier name from
the type system.

**A19.** So the macro behaves as a single statement: `if (x) CHECK(y); else ...`
works, the trailing semicolon is required and natural, and `break`/`continue`
inside cannot leak out. A bare `{ ... }` block would break the `if/else`.

**A20.** `std::source_location` with
`= std::source_location::current()` as a **default argument**. It works because
default arguments are evaluated at the call site, so `current()` captures the
caller's file/line/function — which is exactly what the macros were emulating.

**A21.** Test whether a header exists before including it, so you can degrade
gracefully across toolchains/standard-library versions:
`#if __has_include(<expected>)`. Its siblings are `__has_cpp_attribute` and the
`__cpp_lib_*` feature-test macros in `<version>`.

**A22.** An include guard is standard C++, works everywhere, and survives the same
header being reachable through two different paths (symlinks, copies) — but
requires a unique macro name and the preprocessor must open and scan the file each
time (most compilers have a "multiple include optimization" that recognizes the
pattern). `#pragma once` is non-standard but universally supported, cannot collide,
is less typing, and lets the compiler skip the file by inode — but it can fail to
deduplicate when the same file is visible under two paths (bind mounts, hardlinks).
In practice: use `#pragma once`, or both.

---

**B1.** `data` is a local; the closure captures it by reference and outlives the
function → dangling. Capture by move: `[data = std::move(data)]`.

**B2.** `[this]` captures a raw pointer. If the `Poller` is destroyed before the
pool runs the task, `drain(queue_)` is a use-after-free — a very common async bug.
Fix: derive from `enable_shared_from_this` and capture
`[self = shared_from_this()]`, or capture a `std::weak_ptr` and `lock()` inside,
or guarantee (and document, and assert) that the pool is joined in the destructor.

**B3.** Returns a view into a temporary `std::string` destroyed at the end of the
return statement → dangling `string_view`. Return `std::string` by value.

**B4.** The default argument is repeated in the definition — that is a
redefinition of the default argument and a compile error (gcc:
"default argument given for parameter 2"). Declare it once, in the header.

**B5.** Does not compile: `operator()` is `const`, so `++n` on the by-copy capture
`n` is ill-formed. Add `mutable`: `[n = 0]() mutable { return ++n; }`.

**B6.** Compiles silently — `30` converts to `30.0`. Make the unit part of the
type: `void set_speed(MetersPerSecond)` with an explicit strong type, or
`std::chrono`-style units, and `void set_speed(double) = delete;` to force call
sites to be explicit. A units bug is exactly the class of defect this role cares
about.

**B7.** `compute_expensive()` (or `fallback()`) is evaluated **twice** — once in
the condition and once in the selected branch — so you pay double and, if it has
side effects or is non-deterministic, you get a wrong answer. `std::max` evaluates
each argument once. (Also `MAX` breaks on mixed types and on `std::max`-shadowing
Windows macros.)

**B8.** Each call constructs a temporary `std::string` from the literal. `"tick"`
is 4 chars so libstdc++'s SSO avoids the allocation here, but it still runs a
constructor and destructor a million times — and the moment the literal exceeds
15 characters it becomes a million `malloc`/`free` pairs. Take
`std::string_view`.

**B9.** The **non-template** `print(const std::vector<int>&)` wins — exact match,
and non-template beats template on a tie. That is usually what the author wanted,
but it is worth knowing that adding a `const std::vector<long>&` argument silently
switches to the template, and that if the non-template were declared *after* the
call site in a different TU you would get a different answer (ODR hazard).

**B10.** All three return **garbage / dangle**: `i` is the loop variable, destroyed
when the loop ends, and each closure holds a reference to it. Even during the loop
all three would see the same current `i`. Capture by value: `[i]`.

---

**C1.**
```cpp
template <typename F>
class ScopeGuard {
    F    action_;
    bool active_{true};
public:
    explicit ScopeGuard(F action) noexcept : action_{std::move(action)} {}
    ~ScopeGuard() noexcept { if (active_) action_(); }   // must not throw
    void dismiss() noexcept { active_ = false; }

    ScopeGuard(const ScopeGuard&)            = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&& o) noexcept
        : action_{std::move(o.action_)}, active_{o.active_} { o.dismiss(); }
    ScopeGuard& operator=(ScopeGuard&&) = delete;
};
template <typename F> ScopeGuard(F) -> ScopeGuard<F>;   // CTAD

// usage
void write_frame(int fd) {
    auto* buf = acquire();
    ScopeGuard release_buf{[&] { release(buf); }};       // runs on ANY exit path
    if (!send(fd, buf)) return;                          // including early return
    release_buf.dismiss();                               // ...unless we hand it off
}
```
Why a template parameter: the callable's type is known, so the destructor body
**inlines to the action itself** — zero overhead, no allocation, no indirect call.
A `std::function` member would cost 32 bytes, a possible allocation, and an
indirect call on every scope exit, in a type whose entire purpose is to be free.
Why `noexcept` destructor: destructors run during stack unwinding; throwing from
one while another exception is in flight calls `std::terminate`. So the action
must swallow or log its own errors. Mention `std::experimental::scope_exit` /
`absl::Cleanup` / `folly::ScopeGuard` as the real implementations, and that this
is the RAII answer to "how do you get `finally` in C++".

**C2.**
```cpp
template <typename F, typename... Args>
bool retry(int attempts, F&& fn, Args&&... args) {
    for (int i = 0; i < attempts; ++i) {
        // Forward on every attempt: safe as long as the arguments are not moved
        // from. If fn takes by value and moves, forwarding twice is a bug -- which
        // is exactly the subtlety to call out.
        if (std::invoke(fn, args...)) return true;
    }
    (void)sizeof...(args);
    return false;
}
```
Points to volunteer: use `std::invoke` so it also works with pointer-to-member;
forward `fn` but pass `args` as lvalues inside the loop because a forwarded
argument may be moved-from after the first attempt; a real implementation adds
backoff (`std::this_thread::sleep_for`) and a predicate for "which failures are
retryable".

**C3.**
- `std::vector<std::function<void(const Frame&)>>`: 8 indirect calls, 8 × ~32 bytes
  of state scattered across the heap, likely 8 cache misses per tick, no inlining.
  At 1 kHz that is ~8k indirect calls/s — negligible in absolute terms (~µs), so
  **say that first**: this is almost certainly fine, and reaching for something
  clever without a measurement is the wrong instinct.
- Function pointer + `void* ctx`: one indirection, no allocation, C-compatible,
  cache-friendly if the array is contiguous. Still no inlining. This is what a
  C driver API looks like and it is a perfectly good answer.
- Compile-time `std::tuple` of callables + `std::apply`: fully inlined, zero
  indirection, best possible codegen — but the subscriber set must be known at
  compile time, so no runtime registration, and it bloats binary size and build
  time.

Pick: **`std::function` vector until it shows up in a profile**, then function
pointer + context (keeps runtime registration, removes allocation and 24 bytes per
entry), and reserve the tuple version for a genuinely fixed pipeline. Numbers to
quote: virtual/indirect call ≈ 1-5 ns when the target is in i-cache and the branch
predictor is warm, ~20-40 ns on a miss; `std::function` construction with a
capture > 16 bytes = one `malloc` ≈ 50-100 ns. Against a 1 ms period, 8 callbacks
cost well under 0.1% — so the honest answer is "measure, and spend the effort on
the 1 ms of actual work instead."

**C4.**
```cpp
// Before: 5 arguments, 3 of them coupled, an int return code, no ownership or
// bounds information, silently truncating int lengths.
int process(const float* data, int len, float* out, int out_cap, int* out_len);

// After:
[[nodiscard]] std::expected<std::span<float>, ProcessError>
process(std::span<const float> input, std::span<float> output) noexcept;
```
Justification, item by item:
- `std::span<const float> input` — one argument instead of two that can disagree;
  `const` states that the input is not modified; accepts `vector`, `array`, C
  array, or a subrange; carries `size()` so no separate length to get wrong; and
  `int len` could not even represent a buffer over 2 GB.
- `std::span<float> output` — same, and the *capacity* is now inseparable from the
  pointer, so `out_cap` cannot be stale.
- Return `std::expected<std::span<float>, ProcessError>` — the written subrange is
  returned instead of an out-parameter `int*`, and the error is a typed enum
  instead of a magic `int`. `[[nodiscard]]` makes ignoring it a warning.
- `noexcept` if the function genuinely cannot throw (no allocation) — that is a
  real codegen and API-contract improvement for a foundations library.
- Keep a thin `extern "C"` shim with the original signature if C callers exist;
  the C++ layer is what everyone else uses.
- Alternative if the output size is not known in advance: return
  `std::expected<std::vector<float>, ProcessError>` — but then you have allocated,
  which a real-time caller may not accept. Say that trade-off out loud: **the
  span-in/span-out form is what lets the caller own the memory strategy**, which is
  precisely the job of a foundations API.
