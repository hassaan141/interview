# 01 — C++ Language Rapid Fire

100 questions. Answer each in under 30 seconds, out loud. Anything you hedge on goes on
the re-drill list. Deeper treatment of every topic is in `../../cpp-course/`.

---

## A. Types, values, arithmetic  (course section 01)

1. Difference between `int`, `int32_t`, and `int_fast32_t`?
2. Is `char` signed or unsigned?
3. What is `size_t` and why must a container index use it?
4. Signed overflow vs. unsigned overflow?
5. Why can the compiler delete `if (x + 1 < x)`?
6. What is integral promotion?
7. Why is `-1 < 1u` false?
8. What is `std::cmp_less` for?
9. How many mantissa bits does a `float` have?
10. Name three properties of `NaN`.
11. Is floating-point addition associative? Consequence?
12. What is machine epsilon?
13. How do you compare two doubles correctly?
14. What does `-ffast-math` break?
15. What is a denormal and why is it slow?
16. `auto` — name three things it strips.
17. Why is `auto b = vec_of_bool[0]` not a `bool`?
18. What does `decltype((x))` give for a variable `x`?
19. What does `<=>` give you? Which orderings exist?
20. Which binds tighter, `&` or `==`?

## B. Memory, pointers, lifetime  (course sections 03, 09, 10)

21. Five differences between a pointer and a reference.
22. What is `sizeof(r)` for `int& r`?
23. Wild vs. dangling vs. null pointer?
24. When is pointer arithmetic UB?
25. Cost of a stack allocation vs. a heap allocation?
26. What happens on stack overflow? Is there an exception?
27. `delete` vs `delete[]`? Is `delete nullptr` safe?
28. What is placement new for?
29. `int x;` at block scope — what is its value?
30. `vector<int> v{3,0}` vs `v(3,0)`?
31. What is the most vexing parse?
32. Read out: `const int* p`, `int* const p`.
33. `const` vs `constexpr` vs `consteval` vs `constinit`?
34. What does `if constexpr` do that a runtime `if` cannot?
35. What is `volatile` for? Why is it not a threading tool?
36. Name all five casts and one use of each.
37. What is strict aliasing? Two legal ways to type-pun?
38. What does `dynamic_cast` cost and require?
39. Why is `sizeof(struct)` a multiple of its alignment?
40. What is false sharing?

## C. Classes, RAII, special members  (course section 05)

41. Define RAII in one sentence.
42. In what order are bases, members, and the body initialized?
43. Init-list order or declaration order?
44. What happens if you call a virtual from a constructor?
45. List the six special member functions.
46. What does declaring a destructor do to the move operations?
47. What does declaring a move operation do to the copies?
48. Rule of zero / three / five?
49. `= default` in-class vs. out-of-line?
50. Why must a polymorphic base have a virtual destructor?
51. What is `mutable` for?
52. Is `const` on a method deep or shallow?
53. What is a hidden friend?
54. What does `explicit` do on a conversion operator?
55. Why should move operations be `noexcept`?

## D. Polymorphism and templates  (course sections 06, 07)

56. What is in a vtable? How big is a vptr?
57. What is the *dominant* cost of a virtual call?
58. When does the compiler devirtualize?
59. Why is `final` a performance annotation?
60. What does `override` protect you from?
61. Why should a virtual function never have a default argument?
62. What is name hiding? How do you undo it?
63. Define aggregate / trivially copyable / standard layout.
64. What does trivially copyable buy you?
65. What is the empty base optimization?
66. Can a function template be partially specialized?
67. Why does `typename T::value_type` need `typename`?
68. Why does an unqualified call fail in a class derived from `Base<T>`?
69. Spell out SFINAE. Where does it apply?
70. What are the four kinds of requirement in a `requires` block?
71. When does "more constrained wins" silently fail?
72. What is a fold expression? Which operators have an empty-pack identity?
73. `std::decay_t` vs `std::remove_cvref_t`?
74. What is CTAD?

## E. Move semantics  (course section 09)

75. Name the five value categories.
76. Is `std::move(x)` an lvalue, xvalue, or prvalue?
77. What is `std::move` implemented as?
78. Why is a named rvalue reference an lvalue?
79. What state is a moved-from object in?
80. What happens when you `std::move` a `const` object?
81. What is a forwarding reference? Give a `T&&` that is not one.
82. State the reference collapsing rules.
83. `std::move` vs `std::forward` — when each?
84. Why can you only forward an argument once?
85. Is RVO mandatory? Is NRVO?
86. Why is `return std::move(local);` wrong?
87. Is returning a large object by value slow?

## F. Library, errors, concurrency  (course sections 10, 11, 12)

88. `vector` vs `list` — which wins at middle insertion, and why?
89. When are `vector` iterators invalidated? References?
90. What does `unordered_map` rehash invalidate?
91. What is wrong with `map::operator[]`?
92. What does `std::remove_if` actually do?
93. `std::accumulate` vs `std::reduce`?
94. What is `std::nth_element` for?
95. `sizeof(shared_ptr)`? What is in the control block?
96. What exactly is thread-safe about `shared_ptr`?
97. What is the runtime cost of an exception that is not thrown?
98. What happens if a `noexcept` function throws?
99. Define a data race. What is the consequence in the standard?
100. Explain release/acquire in two sentences.

---
---

# Answers

**A1.** `int` is at least 16 bits (32 everywhere that matters) with an
implementation-defined size; `int32_t` is **exactly** 32 bits and only exists if the
platform has such a type; `int_fast32_t` is **at least** 32 bits, chosen for speed (often
64). Use `int32_t` for ABI/wire layout, `int_fast32_t` for a loop counter.

**A2.** Implementation-defined — signed on x86 Linux, **unsigned** on ARM Linux. That is a
real portability bug when you index an array with a `char` holding a byte > 127. Use
`std::uint8_t`/`std::int8_t` when you mean a number and `char` only for text.

**A3.** An unsigned integer type large enough to hold the size of any object; it is what
`sizeof` and `.size()` return. Using `int` invites sign-conversion warnings, truncation on
huge buffers, and the mixed-sign comparison trap.

**A4.** Signed overflow is **undefined behavior** — the optimizer assumes it cannot
happen. Unsigned overflow is well defined: it wraps modulo 2^N.

**A5.** Because it can only be true if signed overflow occurred, and UB "cannot happen",
so the compiler folds it to `false`.

**A6.** Any integral type with rank below `int` (`bool`, `char`, `short`, bitfields) is
converted to `int` before an arithmetic operation.

**A7.** `int` and `unsigned int` have the same rank, so the signed operand converts to
unsigned: `-1` becomes `4294967295`.

**A8.** Comparing integers by their **mathematical** value regardless of signedness,
without the conversion trap. `<utility>`, C++20.

**A9.** 23 stored, 24 effective with the implicit leading 1 → ~7 decimal digits.

**A10.** `NaN != NaN` is true and every ordered comparison with it is false; it propagates
through arithmetic; there are quiet and signalling NaNs and many bit patterns. Detect with
`std::isnan`.

**A11.** No — each operation rounds. Consequence: vectorized and multithreaded reductions
produce different (equally valid) results from the serial version, so bit-exact replay
requires a fixed reduction order.

**A12.** The gap between 1.0 and the next representable value (2.22e-16 for `double`). It
is **relative**, not an absolute error bound.

**A13.** A mixed tolerance: `diff <= atol || diff <= rtol * max(|a|,|b|)`, plus explicit
NaN and infinity handling.

**A14.** It assumes no NaN or infinity (so `std::isnan` and `x != x` checks fold away),
allows reassociation, ignores signed zero, and sets FTZ/DAZ. Results stop being
reproducible.

**A15.** A subnormal has a zero exponent field and gradual underflow near zero. Some CPUs
handle them in microcode at 10-100x the cost, which is why FTZ modes exist.

**A16.** Top-level `const`/`volatile`, references, and array/function decay to pointer.

**A17.** `std::vector<bool>` is bit-packed; `operator[]` returns a proxy reference object,
and `auto` deduces the proxy.

**A18.** `int&` — the extra parentheses make it an expression, and a named variable used
as an expression is an lvalue.

**A19.** The three-way comparison. `strong_ordering` (equivalent means substitutable),
`weak_ordering` (equivalent but distinguishable), `partial_ordering` (allows unordered —
floats, because of NaN).

**A20.** `==` binds tighter than `&`, so `mask & 1 == 1` means `mask & (1 == 1)`.
Parenthesize bitwise operations.

**B21.** A reference must be initialized, cannot be reseated, cannot legally be null, has
no arithmetic, `sizeof` reports the referent's size, and you cannot have an array of
references.

**B22.** `sizeof(int)` = 4. `sizeof` on a reference reports the referred-to type.

**B23.** Wild = never initialized. Dangling = the pointed-to object's lifetime ended.
Null = `nullptr`, well defined until you dereference it.

**B24.** When the result leaves `[array, array+n]` (one past the end is legal to form,
illegal to dereference), or when the pointer does not point into an array object.

**B25.** Stack: effectively zero marginal cost — the whole frame is one `sub rsp, N`.
Heap: tens to hundreds of instructions, possibly a lock and a syscall, plus a likely cache
miss. Roughly 1 ns vs. 50-200 ns.

**B26.** You hit a guard page and get `SIGSEGV`. There is **no** `std::bad_alloc` and no
diagnosable error — which is why deep recursion and large stack arrays are banned in
real-time code.

**B27.** They must match: `delete` for `new`, `delete[]` for `new[]`; mismatching is UB.
`delete nullptr` is a guaranteed no-op.

**B28.** Constructing an object in storage you already own, with no allocation — the basis
of object pools, fixed-capacity containers, `std::optional` and `std::variant`. You must
call the destructor explicitly.

**B29.** Indeterminate; reading it is UB. At namespace/static/thread scope it is
zero-initialized.

**B30.** `{3,0}` is 2 elements `[3,0]` (the `initializer_list` constructor wins); `(3,0)`
is 3 elements all zero.

**B31.** Anything that could be parsed as a function declaration is. `Widget w();`
declares a function. Use braces.

**B32.** `const int* p` = pointer to a const int (cannot write `*p`, can reseat `p`).
`int* const p` = const pointer to int (can write `*p`, cannot reseat).

**B33.** `const` = cannot be modified after initialization, possibly initialized at
runtime. `constexpr` = usable in a constant expression, implies `const` on a variable.
`consteval` = an immediate function, **must** be evaluated at compile time. `constinit` =
constant-**initialized** (so no static-init-order fiasco) but still mutable.

**B34.** The discarded branch is **not instantiated**, so it may contain code that would be
ill-formed for that type.

**B35.** It tells the compiler the value can change outside the program's control, so
every access must be emitted in order with no caching in a register. It is for MMIO
registers and `volatile sig_atomic_t`. It gives **no atomicity** and **no fences**, so it
creates no happens-before edge — it is still a data race.

**B36.** `static_cast` (numeric conversion, unchecked downcast), `const_cast` (a legacy C
API that takes `char*`), `reinterpret_cast` (an integer address to a `volatile uint32_t*`
for MMIO), `dynamic_cast` (a plugin boundary where you must query the dynamic type),
`std::bit_cast` (reading a float's IEEE bits).

**B37.** The rule that an object may only be accessed through a glvalue of its own type
(or a cv/sign variant, or `char`/`unsigned char`/`std::byte`). Legal punning:
`std::bit_cast` and `std::memcpy`.

**B38.** A polymorphic type and RTTI enabled; it costs a runtime walk of the type-info
graph (tens of nanoseconds) and cannot be inlined.

**B39.** So that `arr[i]` in an array of that struct is still correctly aligned — hence
trailing padding.

**B40.** Two threads writing different variables that share a 64-byte cache line, so the
line ping-pongs through the coherence protocol. Fix with
`alignas(std::hardware_destructive_interference_size)`; detect with `perf c2c`.

**C41.** Every resource is owned by an object that acquires it in its constructor and
releases it in its destructor, so the resource's lifetime is tied to a scope and cannot
leak on any exit path, including an exception.

**C42.** Virtual bases, then direct bases in declaration order, then non-static data
members in declaration order, then the constructor body. Destruction is exactly reversed.

**C43.** **Declaration** order, always. `-Wreorder` warns when the init-list disagrees.

**C44.** Dispatch resolves to the **current** class's override — the derived part does not
exist yet and its vptr is not set. A pure virtual call there is UB.

**C45.** Default constructor, copy constructor, copy assignment, move constructor, move
assignment, destructor.

**C46.** It **suppresses** them, so every "move" silently performs a copy. The traits still
report the type as movable, which is why the bug is invisible.

**C47.** It **deletes** the copy constructor and copy assignment.

**C48.** Rule of zero: own resources through RAII members and declare none of the six.
Rule of three: if you need one of destructor/copy-ctor/copy-assign, you need all three.
Rule of five: with moves, declare all five or none.

**C49.** In-class keeps the function **trivial** (so the type can stay trivially copyable);
out-of-line makes it user-provided and non-trivial.

**C50.** Because `delete` through a base pointer to a derived object is UB without it —
the derived destructor never runs. Alternative: a `protected` non-virtual destructor.

**C51.** Allowing a member to be modified in a `const` method. Legitimate uses: a
memoization cache, a `std::mutex`, an atomic counter — things not part of the observable
value.

**C52.** **Shallow.** A `T*` member becomes `T* const`, so the pointee stays writable.

**C53.** A `friend` function **defined inside** the class body. It is only findable by ADL,
keeps overload sets small, and gives symmetric conversions for binary operators.

**C54.** It blocks implicit use except in a boolean context — which is exactly why
`explicit operator bool()` gives you `if (ptr)` without `int n = ptr;`.

**C55.** Because `std::vector` uses `std::move_if_noexcept` on reallocation: a throwing
move forces it to **copy** every element to preserve the strong guarantee.

**D56.** Offset-to-top, a pointer to the `type_info`, then the virtual function pointers in
declaration order (the destructor takes two slots). A vptr is one pointer, 8 bytes on LP64.

**D57.** Not the indirect jump — that the compiler **cannot inline through it**, so it
loses every downstream optimization.

**D58.** When it can prove the dynamic type: a local object, a `final` class or method, LTO
with hidden visibility, or speculative devirtualization from a PGO profile.

**D59.** Because it proves no further override exists, so a call through a base pointer has
one possible target and can be inlined.

**D60.** A signature mismatch (a missing `const`, a wrong parameter type) that silently
creates a new function hiding the base one instead of overriding it.

**D61.** Default arguments are resolved from the **static** type while the body is chosen
by the **dynamic** type, so you get the derived implementation with the base's default.

**D62.** Declaring any name in a derived class hides **all** base declarations of that
name, including different arities. Undo with `using Base::f;`.

**D63.** **Aggregate**: no user-provided constructors, no private/protected non-static data
members, no virtuals — brace-initializable. **Trivially copyable**: all copy/move
operations and the destructor are trivial — `memcpy` is a valid copy. **Standard layout**:
one access-control group, no virtuals, at most one class in the hierarchy with data
members — C-compatible offsets.

**D64.** You can `memcpy` it into shared memory, DMA it, or put it in a lock-free ring, and
`std::vector` can relocate it with `memmove`.

**D65.** A base-class subobject need not have a unique address, so an empty base can take
zero bytes — which is how `unique_ptr<T, StatelessDeleter>` is one pointer wide. C++20
`[[no_unique_address]]` extends it to members.

**D66.** **No** — only fully. Use overloads (which participate in partial ordering), tag
dispatch, `if constexpr`, or delegate to a class template.

**D67.** Because it is a **dependent** name and the compiler must decide at parse time
whether it names a type or a value; the default assumption is "not a type".

**D68.** Unqualified lookup does not search **dependent** base classes. Use `this->f()`,
`Base<T>::f()`, or a using-declaration.

**D69.** Substitution Failure Is Not An Error: an invalid type or expression produced while
substituting into a candidate's **signature** removes it from the overload set instead of
being an error. Failures in the **body** are hard errors.

**D70.** Simple (an expression must be valid), type (`typename T::value_type;`), compound
(`{ expr } noexcept -> Concept;`), and nested (`requires OtherConcept<T>;`).

**D71.** When the constraints are not related after normalization — in particular a concept
written as one opaque atom over a type trait (`concept A = std::is_arithmetic_v<T>;`) does
not subsume `std::integral`, so you get ambiguity. Build concepts out of concepts.

**D72.** A pack expansion over a binary operator. Only `&&` (identity `true`), `||`
(`false`) and `,` are valid on an **empty** pack; `+` and `*` are not.

**D73.** `decay_t` also applies array-to-pointer and function-to-pointer decay (it models
pass-by-value); `remove_cvref_t` only strips references and cv. In a forwarding context you
usually want `remove_cvref_t`.

**D74.** Class template argument deduction: `std::vector v{1,2,3}` deduces
`std::vector<int>`. Deduction guides let you add or correct the synthesized rules.

**E75.** lvalue, prvalue, xvalue, plus the unions glvalue and rvalue. The two properties are
identity and movability.

**E76.** An **xvalue** (it returns `T&&`).

**E77.** `static_cast<std::remove_reference_t<T>&&>(t)` — a `constexpr noexcept` cast that
emits zero instructions.

**E78.** Value category is a property of the **expression**, not the type. Once the
reference has a name, using that name has identity and is not automatically movable.

**E79.** Valid but **unspecified**. You may destroy it or assign to it; you must not read
its value or call an operation with a precondition on it.

**E80.** You get `const T&&`, which binds to `const T&`, so you silently get a **copy**.

**E81.** `T&&` where `T` is a template parameter **being deduced by that function** (or
`auto&&`). Not forwarding: `void f(Widget&&)`, and `template<class T> void f(vector<T>&&)`.

**E82.** `T& &` → `T&`; `T& &&` → `T&`; `T&& &` → `T&`; `T&& &&` → `T&&`. An lvalue
reference anywhere wins.

**E83.** `std::move` for a named rvalue-reference parameter or a local you are done with;
`std::forward<T>` for a forwarding reference whose category you must preserve.

**E84.** Because forwarding may transfer ownership: the first forward can move out, leaving
the object empty for the second.

**E85.** RVO (returning an unnamed temporary) is **mandatory** since C++17. NRVO (returning
a named local) is permitted but **not** required — universal in practice, disabled by
`-fno-elide-constructors`.

**E86.** It turns an id-expression that NRVO could elide entirely into an xvalue that
cannot be elided, forcing a move that would have been zero operations.
`-Wpessimizing-move` flags it.

**E87.** **No.** For a prvalue it is guaranteed zero copies and zero moves; for a named
local NRVO does the same, and if it cannot, you get a move.

**F88.** `vector`, almost always — its O(n) `memmove` runs at tens of GB/s with perfect
prefetching, while `list`'s O(1) insert requires an O(n) walk of dependent pointer loads,
each a potential ~100 ns cache miss, plus one allocation per node.

**F89.** Iterators and references are both invalidated by any reallocation, and from the
modification point onward by `insert`/`erase`.

**F90.** All **iterators**, but **no references or pointers** — it is node-based, so
rehashing relinks nodes without moving elements.

**F91.** It is non-`const` and it **inserts** a default-constructed value when the key is
absent. Use `find`, `at`, or `contains`.

**F92.** It **partitions**: it moves the kept elements forward and returns the new logical
end. The container's size is unchanged — you must `erase`, or use `std::erase_if`.

**F93.** `accumulate` applies the operation strictly left-to-right; `reduce` may reorder
and parallelize, so it needs associativity and gives different floating-point results.

**F94.** Partially sorting so the element at position n is the one that would be there in a
sorted range — **O(n) average**. The right tool for a median or top-k.

**F95.** Two pointers (object + control block). The control block holds the strong count,
the weak count, the deleter, and the allocator.

**F96.** The **control block** — you may copy and destroy `shared_ptr`s to the same object
from many threads. The **pointee** is not, and neither is writing the same `shared_ptr`
object from two threads (use `std::atomic<std::shared_ptr<T>>`).

**F97.** **Zero.** The Itanium zero-cost EH model puts the unwind data in a separate table;
there is no check and no branch on the happy path.

**F98.** `std::terminate` is called, and stack unwinding is not guaranteed to have
happened.

**F99.** Two threads access the same memory location, at least one writes, and there is no
happens-before relationship between them. The consequence is **undefined behavior**, not a
stale value.

**F100.** A release store and an acquire load on the same atomic create a synchronization
edge: if the acquire load reads the value written by the release store, everything
sequenced before the store in the writing thread happens-before everything after the load
in the reading thread. That is how you publish data without a lock.
