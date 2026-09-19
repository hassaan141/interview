# 09 — Mock interview questions

"Explain move semantics" is asked in essentially every C++ interview. Rehearse A1-A6
until they are one fluent paragraph.

## A. Rapid fire

1. Name the five value categories and the two properties that define them.
2. Is `std::move(x)` an lvalue, xvalue, or prvalue? What about `x + 1`? `*p`?
3. What is `std::move` actually implemented as? How many instructions does it emit?
4. Why is a **named** rvalue reference an lvalue?
5. What state is a moved-from object in? What may you do with it?
6. Is a moved-from `std::unique_ptr` guaranteed null? A moved-from `std::string`?
7. What happens when you `std::move` a `const` object?
8. Why must move operations be `noexcept`?
9. What is a forwarding reference? Give an example of a `T&&` that is *not* one.
10. State the reference collapsing rules.
11. What is the special deduction rule that makes forwarding references work?
12. `std::move` vs `std::forward` — when do you use each?
13. Why must you pass the template argument to `std::forward` explicitly?
14. Why can you only forward an argument once?
15. What is RVO? Is it mandatory? What about NRVO?
16. Why is `return std::move(local);` wrong?
17. Why does `return flag ? a : b;` copy while `if (flag) return a; return b;` moves?
18. Is returning a large object by value slow? Explain.
19. What problem does a forwarding-reference constructor cause, and how do you fix it?
20. What are ref-qualifiers on a member function for?
21. `auto` vs `auto&` vs `auto&&` vs `decltype(auto)`.
22. What is `auto(x)` in C++23 for?
23. Is `const` deep or shallow? What does `const` buy you at the ABI/optimizer level?

## B. Find the bug / count the operations

**B1.**
```cpp
void consume(std::string&& s) { store(s); }      // store takes std::string
```

**B2.**
```cpp
std::vector<std::string> v;
std::string s = "long string that exceeds SSO";
v.push_back(std::move(s));
std::cout << s.size();
```

**B3.**
```cpp
template <typename T> void wrapper(T&& x) {
    logger(std::forward<T>(x));
    process(std::forward<T>(x));
}
```

**B4.**
```cpp
class Widget {
    std::vector<int> data_;
public:
    Widget(std::vector<int> d) : data_{d} {}     // how many copies for
};                                               // Widget w{make_vector()}?
```

**B5.**
```cpp
std::string build() {
    std::string result;
    result += "a";
    return std::move(result);
}
```

**B6.**
```cpp
struct Frame { std::vector<std::uint8_t> px; ~Frame() = default; };
std::vector<Frame> frames;
frames.push_back(capture());
```

**B7.**
```cpp
class Name {
    std::string s_;
public:
    template <typename T> Name(T&& t) : s_{std::forward<T>(t)} {}
};
Name a{"x"};
Name b{a};
```

**B8.**
```cpp
std::vector<int> v{1,2,3,1};
std::erase(v, v.front());
```

**B9.**
```cpp
auto&& ref = std::vector<int>{1,2,3};
auto&& elem = make_widget().field;
```

**B10.**
```cpp
std::unique_ptr<Frame> p = std::make_unique<Frame>();
process(std::move(p));
if (p) p->tick();
```

## C. Whiteboard

**C1.** Write a `SmallVector<T, N>` move constructor that handles both the
inline-storage and heap-storage cases. Explain why it cannot be `noexcept`
unconditionally.

**C2.** A zero-copy pipeline: `capture() -> undistort() -> detect()`, each stage
producing a large buffer. Write the signatures so no buffer is ever copied, and say
where each move happens.

**C3.** `std::vector<T>::push_back` — walk through every operation, including
reallocation, for (a) a `T` with a `noexcept` move, (b) a `T` with a throwing move,
(c) a trivially copyable `T`. Explain the exception guarantee in each case.

**C4.** Implement `emplace_back` for a simple vector. Why is it different from
`push_back(T&&)`, and when does it actually win?

---
---

# Answers

**A1.** lvalue, prvalue, xvalue, and the two unions glvalue (= lvalue ∪ xvalue) and
rvalue (= prvalue ∪ xvalue). The two properties are **identity** (does the
expression name a specific object you could take the address of?) and **movability**
(may its resources be stolen?). lvalue = identity, not movable. prvalue = no
identity, movable. xvalue = identity **and** movable.

**A2.** `std::move(x)` is an **xvalue** (it returns `T&&`). `x + 1` is a **prvalue**.
`*p` is an **lvalue**. Quick test: `decltype((e))` yields `T&` for an lvalue, `T&&`
for an xvalue, and plain `T` for a prvalue.

**A3.** `static_cast<std::remove_reference_t<T>&&>(t)` — a `constexpr noexcept`
cast. It emits **zero** instructions. It changes nothing at runtime; it only changes
which overload the compiler selects. Saying "`std::move` doesn't move anything" is
the answer they are listening for.

**A4.** Because value category is a property of the *expression*, not the type.
`T&&` describes what the reference can bind to; once the reference has a **name**,
using that name is an expression that has identity and is not automatically
movable — otherwise every use of the parameter inside the function could silently
steal from it. Hence you must write `std::move(s)` again at the point you really
want to transfer.

**A5.** **Valid but unspecified.** You may call any operation whose preconditions you
can satisfy without knowing the value — destroy it, assign a new value, call
`clear()`, `size()`, `empty()`. You must **not** read its value and assume anything,
and you must not call an operation with a precondition on the value (e.g.
`front()`).

**A6.** A moved-from `std::unique_ptr` **is** guaranteed to be `nullptr` (the
standard specifies it). A moved-from `std::string`/`std::vector` is only "valid but
unspecified" — in practice empty in every implementation, but not guaranteed, and
short strings may not be cleared because of SSO.

**A7.** You get `const T&&`, which **cannot** bind to a `T&&` parameter, so overload
resolution falls back to `const T&` and you get a **copy** — silently, with no
warning. `const` kills moves. This is a common cause of "I added `std::move` and it
got no faster".

**A8.** Because `std::vector` uses `std::move_if_noexcept` when it reallocates: if
`T`'s move constructor can throw, the vector must **copy** every element in order to
preserve the strong exception guarantee (it cannot un-move the elements it already
moved). So a non-`noexcept` move is a silent performance cliff on every growth.
`noexcept` also enables better codegen (no unwind path) and is required by several
standard-library preconditions.

**A9.** A parameter of the form `T&&` where `T` is a template parameter **being
deduced by that same function** (or `auto&&`). Not forwarding references:
`void f(Widget&&)` (no deduction), `template <typename T> void f(std::vector<T>&&)`
(the `&&` is not applied directly to `T`), and `T&&` in a member function of a class
template where `T` is the *class's* parameter (already fixed).

**A10.** `T& &` → `T&`; `T& &&` → `T&`; `T&& &` → `T&`; `T&& &&` → `T&&`. An lvalue
reference anywhere in the collapse wins.

**A11.** For a parameter `T&&`: if the argument is an **lvalue** of type `U`, `T` is
deduced as `U&` (so the parameter collapses to `U&`); if it is an **rvalue**, `T` is
deduced as `U` (parameter is `U&&`). That single asymmetry, plus collapsing, is the
whole mechanism.

**A12.** `std::move` for a **named rvalue reference parameter** or a local you are
done with — you know it is safe to steal. `std::forward<T>` for a **forwarding
reference**, where you must preserve whatever the caller gave you. Rule of thumb:
`T&&` deduced → `std::forward<T>`; `Widget&&` concrete → `std::move`.

**A13.** Because `std::forward`'s parameter is
`std::remove_reference_t<T>&`, which is a non-deduced context — the whole point is
that the *caller* supplies the category information that the deduced `T` carries.
If `T` could be deduced from the argument, `forward` would always see an lvalue and
could never produce an rvalue. `std::forward(x)` without `<T>` does not compile.

**A14.** Because forwarding may transfer ownership. If the argument was an rvalue,
the first `std::forward` can move out of it, leaving it in a moved-from state; the
second forward then moves from an empty object. Forward exactly once per argument,
on the last use.

**A15.** **RVO** — eliding the copy/move when returning an unnamed temporary
(prvalue). Since **C++17 it is mandatory**: there is no copy or move at all, and the
type does not even need to be movable, because the prvalue initializes the caller's
object directly. **NRVO** — eliding it when returning a **named** local. It is
**permitted but not required**; every mainstream compiler does it (even at `-O0`),
and `-fno-elide-constructors` turns it off. If NRVO does not apply, C++11's implicit
move on return applies instead, so you get a move, not a copy.

**A16.** It turns the return expression from an id-expression naming a local (which
NRVO can elide entirely) into an xvalue, which **cannot** be elided — so you force a
move constructor call that would otherwise have been zero operations. gcc/clang warn
with `-Wpessimizing-move`. The exception: `return std::move(member)` or
`return std::move(global)` where elision was never possible anyway.

**A17.** C++11's implicit-move-on-return rule only applies when the returned
expression is an **id-expression naming an implicitly movable local object**.
`flag ? a : b` is a conditional expression, not an id-expression, so it stays an
lvalue and the copy constructor is selected. `if (flag) return a; return b;` returns
a name each time, so both returns are treated as rvalues and move. (Neither form
gets NRVO, because there are two candidate objects.)

**A18.** **No.** For a prvalue return it is guaranteed zero copies and zero moves
(C++17 RVO) — the object is constructed in place in the caller's storage. For a
named local, NRVO does the same in practice, and if it cannot, you get a move. So
`std::vector<Frame> process();` is as cheap as an out-parameter and far clearer.
The old "return by value is slow" advice predates C++11.

**A19.** A template constructor taking `T&&` is an **exact match** for a non-const
lvalue of the class's own type, while the copy constructor requires adding `const` —
so `Widget b{a};` selects the *template*, which then tries to construct a member
from a `Widget` and produces a bizarre error (or worse, compiles and does the wrong
thing). It also hijacks derived-class copies. Fix: constrain it with
`requires (!std::same_as<std::remove_cvref_t<T>, Widget>)` — and also exclude
derived types with `!std::derived_from<std::remove_cvref_t<T>, Widget>` — or use
`std::enable_if_t` pre-C++20, or just take `std::string` by value and move.

**A20.** They let a member function be selected based on whether `*this` is an
lvalue or an rvalue. The canonical use is "return a view if the object will live on,
steal the buffer if it is about to die":
`const std::string& str() const&` vs. `std::string str() &&`. Also used to *forbid*
calling something on a temporary (`auto data() && = delete;` prevents
`get_vector().data()` from dangling).

**A21.** `auto x = e` — by-value deduction, decays (drops refs, top-level cv, arrays
to pointers). `auto& x = e` — lvalue reference, preserves cv, no decay, will not
bind an rvalue. `auto&& x = e` — a **forwarding reference**: binds anything,
preserves cv and value category, and extends a temporary's lifetime. `decltype(auto)
x = e` — deduces with `decltype` rules, preserving references exactly; sensitive to
`return x;` vs. `return (x);`.

**A22.** `auto(x)` / `auto{x}` is an explicit **decay-copy**: it produces a prvalue
copy of `x` with references and cv stripped. It exists to break aliasing in one
expression, e.g. `std::erase(v, auto(v.front()))` — without the copy, the algorithm
holds a reference into the container it is modifying.

**A23.** **Shallow**: `const` on a member function makes the members `const`, so a
`T*` member becomes `T* const` and the pointee stays writable. What it buys you:
it documents and enforces intent, it is required for a type to be read concurrently
from multiple threads without synchronization (const member functions on standard
types are guaranteed thread-safe for concurrent reads), it allows binding to
temporaries, it enables `const`-overload selection, and it lets the optimizer assume
a `const` object with no address taken and internal linkage never changes (though
`const` alone on a reference parameter does **not** let it assume no aliasing — that
is what `__restrict` is for).

---

**B1.** `s` is a named rvalue reference, so `store(s)` passes an **lvalue** and
copies. Write `store(std::move(s))`. The signature promised the caller you would take
ownership, and then you copied — the worst of both.

**B2.** Legal but meaningless: `s` was moved from, so its value is **unspecified**.
Reading `s.size()` is not UB, but the answer is implementation-defined (0 in
practice). Never read a moved-from object.

**B3.** Forwards twice. If the caller passed an rvalue, `logger` may have moved out
of it, so `process` receives an empty object. Forward only on the **last** use:
`logger(x); process(std::forward<T>(x));`.

**B4.** For `Widget w{make_vector()}`: the by-value parameter `d` is initialized by
**move** (from the prvalue, in fact elided into the parameter — zero cost), then
`data_{d}` **copies** it because `d` is an lvalue. One unnecessary copy. Fix:
`data_{std::move(d)}`. Taking the parameter by value **and** moving from it is the
"sink parameter" idiom and is correct; forgetting the `std::move` is the bug.

**B5.** `std::move` on a local return defeats NRVO, turning zero operations into one
move construction. `-Wpessimizing-move` flags it. Write `return result;`.

**B6.** The user-declared `~Frame() = default;` **suppresses** the implicit move
constructor, so `push_back` copies the pixel buffer, and so does every reallocation.
Delete the destructor declaration entirely (rule of zero) — `= default` in the class
still counts as user-declared for this rule.

**B7.** `Name b{a}` selects the **template** constructor (exact match `Name&`) rather
than the copy constructor (`const Name&` requires a qualification conversion), so it
tries to initialize `std::string s_` from a `Name` and fails to compile — or, with a
more permissive member, silently does something absurd. Constrain the template as in
A19.

**B8.** `v.front()` is a reference **into** `v`. `std::erase` moves elements around
while the reference is still live, so the comparison value changes underneath the
algorithm — undefined behaviour. Fix: copy the value out first (`auto(v.front())` in
C++23, or a named local).

**B9.** Both lines look alike and only the first is safe. `auto&& ref =
std::vector<int>{1,2,3};` binds a reference to a **temporary** and extends its
lifetime to `ref`'s scope — fine. `auto&& elem = make_widget().field;` binds to a
**subobject** of a temporary; lifetime extension does not apply through the member
access before C++23, so the `Widget` dies at the end of the statement and `elem`
dangles.

**B10.** `p` is guaranteed `nullptr` after the move (that is specified for
`unique_ptr`), so `if (p)` is false and `p->tick()` never runs. This one is *not*
undefined — it just silently does nothing, which is arguably worse because it looks
correct. Restructure so ownership transfer is the last thing you do with `p`.

---

**C1.**
```cpp
template <typename T, std::size_t N>
class SmallVector {
    alignas(T) std::byte inline_[N * sizeof(T)];
    T*          data_{reinterpret_cast<T*>(inline_)};
    std::size_t size_{0}, cap_{N};

    bool is_inline() const noexcept { return data_ == reinterpret_cast<const T*>(inline_); }
public:
    SmallVector(SmallVector&& o)
        noexcept(std::is_nothrow_move_constructible_v<T>)   // <- conditional!
    {
        if (o.is_inline()) {
            // Heap storage can be stolen with a pointer swap; INLINE storage cannot
            // -- it lives inside the source object, so every element must be
            // move-constructed into our own inline buffer one at a time.
            for (std::size_t i = 0; i < o.size_; ++i)
                new (data_ + i) T(std::move(o.data_[i]));
            size_ = o.size_;
            cap_  = N;
            o.clear();                       // destroy the husks, size_ = 0
        } else {
            data_ = std::exchange(o.data_, reinterpret_cast<T*>(o.inline_));
            size_ = std::exchange(o.size_, 0);
            cap_  = std::exchange(o.cap_, N);
        }
    }
    ~SmallVector() { clear(); if (!is_inline()) ::operator delete(data_); }
    void clear() noexcept { while (size_) data_[--size_].~T(); }
};
```
Why it cannot be unconditionally `noexcept`: in the inline case the move performs
**N move constructions of `T`**, and if `T`'s move can throw, so can this one. Hence
`noexcept(std::is_nothrow_move_constructible_v<T>)`. Points to volunteer: this is
exactly why `llvm::SmallVector` and `absl::InlinedVector` are not trivially
relocatable, why `std::vector<SmallVector<std::string,4>>` reallocation is more
expensive than you would guess, and why C++26's `std::inplace_vector` specifies its
`noexcept` the same conditional way. Also: the move leaves the source **empty**,
not "unspecified-but-whatever" — being deliberate about that makes the type far
easier to reason about.

**C2.**
```cpp
struct Buffer {                       // rule of zero: movable, not cheap to copy
    std::vector<std::uint8_t> bytes;
    std::uint64_t timestamp_ns{};
};

[[nodiscard]] Buffer capture();                        // returns by value: RVO
[[nodiscard]] Buffer undistort(Buffer in);             // sink by value, return same
[[nodiscard]] Detections detect(const Buffer& in);      // read-only: const&

// call site -- zero buffer copies:
Detections run() {
    Buffer raw   = capture();                 // (1) RVO: constructed in place
    Buffer fixed = undistort(std::move(raw)); // (2) one move into the parameter,
                                              //     then NRVO out
    return detect(fixed);                     // (3) no copy: read-only reference
}
```
Where the moves are: (1) none — mandatory RVO. (2) exactly **one** move
construction, into `undistort`'s by-value parameter; inside it you mutate `in` and
`return in;`, which NRVO elides. (3) none — `detect` only reads.

Alternative worth mentioning, and the one a real autonomy stack uses: **don't move
buffers at all, reuse them.** Pass `std::span<std::uint8_t>` views into a
preallocated pool of buffers and return a small handle, so there is no allocation
*and* no move in the steady state:
```cpp
void undistort(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) noexcept;
```
Say both answers: the value-semantics version is clean and copy-free; the span
version is what you ship in a 100 Hz loop because it has no allocation at all. That
contrast is the point of the question.

**C3.** `push_back(const T&)` / `push_back(T&&)`:
1. If `size() < capacity()`: construct one `T` at `end()` (copy or move as
   appropriate), then `++size_`. If that construction throws, nothing has changed —
   **strong guarantee**, trivially.
2. If `size() == capacity()`: allocate new storage (typically 1.5x or 2x), then
   **transfer** the existing elements, then construct the new element, then destroy
   the old elements and deallocate.

The transfer step is where `noexcept` decides everything:
- **(a) `T` has a `noexcept` move**: `std::move_if_noexcept` moves each element.
  Fast. If the *new element's* construction throws, the vector is left with the
  moved elements in the new buffer — the standard still requires the strong
  guarantee for `push_back`, which implementations get by constructing the new
  element **first**, before relocating. Net: O(n) moves, strong guarantee.
- **(b) `T`'s move can throw**: `move_if_noexcept` falls back to **copying** every
  element, because a throw partway through a move would leave elements destroyed in
  the old buffer with no way to restore them. So you pay n copies instead of n moves
  on every growth — potentially orders of magnitude worse. Strong guarantee
  preserved, at the cost of performance.
- **(c) trivially copyable `T`**: the implementation may relocate with a single
  `memcpy`/`memmove` of the whole block. No per-element calls, no exceptions
  possible, and it vectorizes. This is why `std::vector<POD>` growth is nearly free
  and `std::vector<std::string>` growth is not.

Amortized cost is O(1) per `push_back` because of geometric growth; the fix for the
latency spike is `reserve()`. And the interview-closing line: "`reserve` plus a
`noexcept` move turns the worst case from n copies into zero work."

**C4.**
```cpp
template <typename T>
class Vec {
    T* data_; std::size_t size_, cap_;
public:
    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == cap_) grow();
        T* slot = new (data_ + size_) T(std::forward<Args>(args)...);  // build IN PLACE
        ++size_;
        return *slot;
    }
    void push_back(const T& v) { emplace_back(v); }
    void push_back(T&& v)      { emplace_back(std::move(v)); }
};
```
Difference: `push_back(T&&)` requires a `T` to **already exist**, then moves it into
the container — so constructing from arguments costs one construction **plus one
move**. `emplace_back` forwards the constructor arguments and builds the object
directly in the container's storage — **one construction, zero moves**.

When it actually wins:
- When the element is expensive to move or **not movable at all**
  (`std::mutex`, a type with a deleted move, `std::atomic`).
- When constructing from arguments rather than from an existing object:
  `v.emplace_back("key", 42)` vs. `v.push_back(Pair{"key", 42})`.
- In a map: `m.try_emplace(k, args...)` avoids constructing the value at all when the
  key already exists, which `insert`/`emplace` do not guarantee.

When it does **not** win, which is the part that shows judgment: if you already have
a `T` and move it (`v.emplace_back(std::move(t))` == `v.push_back(std::move(t))`),
or if `T` is trivially copyable (the "move" is a register copy). And one real
downside: `emplace_back` uses **direct**-initialization, so it will happily invoke
an `explicit` constructor or a narrowing conversion that `push_back` would have
rejected — `std::vector<std::unique_ptr<T>> v; v.emplace_back(new T);` compiles and
is a leak hazard, while `push_back(new T)` does not compile.
