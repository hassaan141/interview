# 06 — Build Systems, Toolchain, and Testing

> "evangelize best software practices" and "write tests and integrate with our evaluation
> pipeline"

Both clauses are in the posting. This is the part of the job where you are judged on
opinions, not recall — so have opinions you can defend.

---

## Questions

### Compilation and linking
1. Walk through the build pipeline from `.cpp` to a running process.
2. What is a translation unit, exactly?
3. What does `inline` actually mean?
4. State the One Definition Rule. What is IFNDR?
5. `undefined reference to vtable for X` — what causes it?
6. Why does a template defined in a `.cpp` fail to link? Two fixes.
7. Why does static-library link order matter?
8. Static vs. shared libraries — four trade-offs.
9. Define ABI. Name five changes that break it.
10. What does `-fvisibility=hidden` do and why does it also help performance?
11. What is LTO? ThinLTO? What does LTO commonly *expose*?
12. What is the static initialization order fiasco and how do you fix it?

### CMake
13. `PUBLIC` vs `PRIVATE` vs `INTERFACE`.
14. Why is target-based CMake better than the directory-based style?
15. How do you keep flags identical across every TU, and why does it matter?
16. How would you ship a library that 40 other targets consume?

### Testing
17. What is the test pyramid? What fourth layer does autonomy add?
18. What is your rule about tests when fixing a bug?
19. Why is `EXPECT_EQ` on doubles a broken test?
20. Why is a `sleep()` in a test a bug? What do you do instead?
21. Why must a randomized test be seeded?
22. What is property-based testing? Give an example from this repo.
23. What is fuzzing and why is it the highest-value technique for a parser?
24. Line vs. branch coverage. Why is coverage a bad target?
25. How do you deal with a flaky test suite?

### Sanitizers and static analysis
26. Which sanitizer finds what? Which two cannot be combined?
27. What is the fundamental limitation of all sanitizers?
28. What do `-D_GLIBCXX_ASSERTIONS`, `-D_FORTIFY_SOURCE=3`, and
    `-ftrivial-auto-var-init=zero` cost and buy?
29. Name four clang-tidy check families.
30. Design the CI pipeline for a foundations library. What gates what?

---
---

# Answers

**1.** Preprocess (`#include` expansion, macros, `#if`) → compile to assembly → assemble to
an object file with relocations and a symbol table → link (resolve symbols across objects
and libraries, lay out sections, apply relocations) → at runtime, the dynamic loader maps
the executable and its shared libraries, resolves dynamic symbols (lazily via the PLT/GOT
unless `BIND_NOW`), runs static initializers, then calls `main`.

**2.** One source file **after preprocessing** — the `.cpp` plus every transitively included
header, with macros expanded and `#if` branches resolved. It is what the compiler sees; the
compiler knows nothing about other TUs and the linker knows nothing about types.

**3.** "This entity may be defined in more than one translation unit, and the linker must
merge the definitions into one" — implemented as a weak/COMDAT symbol. It is what makes a
definition legal in a header. It does **not** mean "expand this call at the call site";
that is entirely the optimizer's decision.

**4.** (1) At most one definition per TU of any entity. (2) Exactly one definition in the
whole program of every non-`inline` function or variable that is used. (3) Classes,
`inline` entities, and templates may be defined in multiple TUs **provided every definition
is the same token sequence and means the same thing**. Violating (3) is **IFNDR** —
ill-formed, no diagnostic required: the linker silently keeps one definition and the program
is wrong with no error. `-flto` plus `-Wodr` is the practical detector.

**5.** The compiler emits a class's vtable in the TU that defines its **key function** (the
first non-inline, non-pure virtual, usually the destructor). If every virtual is declared
but none defined, no TU emits the vtable. Fix: define at least one virtual out of line.

**6.** The compiler instantiates a template where it is **used** and can only do so if the
definition is visible in that TU. Fixes: put the definition in the header, or use **explicit
instantiation** (`template class Vec<int>;` in one `.cpp` plus `extern template` in the
header), which also cuts build time.

**7.** The linker processes inputs left to right and extracts an object from an archive only
if it resolves a **currently undefined** symbol. If the archive appears before the object
that needs it, nothing is undefined yet, nothing is extracted, and you get an undefined
reference. Objects first, libraries last; `--start-group/--end-group` for cycles.

**8.** (a) **Size/deployment**: static duplicates code into every consumer but ships one
file; shared is one copy shared across processes but must be found at load time. (b)
**Update**: a shared library can be swapped without relinking *if the ABI holds*. (c)
**Performance**: static allows cross-module inlining and whole-program LTO with no PLT
indirection; shared pays an indirect call per cross-library call and cannot inline across
the boundary. (d) **Determinism**: static has no loader surprises. For an automotive target,
static + LTO is usually right.

**9.** The Application Binary Interface: name mangling, struct layout and alignment, calling
convention, vtable layout, RTTI representation, unwind tables, and the standard library's
own type layouts. Breaking changes: adding/removing/**reordering virtual functions**;
adding/removing/reordering **data members**; changing a type's size or alignment; changing
a function signature or return type; changing the body of an `inline` function others have
already inlined; changing a default argument; changing an enum's underlying type; toggling
`_GLIBCXX_USE_CXX11_ABI`.

**10.** It stops symbols being exported from a shared object unless explicitly marked, which
shrinks the dynamic symbol table (faster load, smaller binary, no accidental ABI surface).
It **helps performance** because the compiler and LTO then know nothing outside can
interpose or override those symbols, so they can inline and devirtualize aggressively —
`-fno-semantic-interposition` is the related flag.

**11.** LTO defers optimization to link time so the compiler sees the whole program:
cross-TU inlining, devirtualization, dead-code elimination, symbol internalization — 5-15%
typical, and usually a *smaller* binary. **ThinLTO** partitions the work so it parallelizes,
getting most of the benefit at a fraction of the link time. LTO commonly **exposes latent
UB** (cross-TU strict aliasing, ODR violations now visible to `-Wodr`) — a test that starts
failing under LTO is usually a real bug, not an LTO bug.

**12.** Namespace-scope objects needing **dynamic** initialization are initialized in
unspecified order **across** TUs, so one global can be used before it is constructed. Fixes,
best first: remove the mutable global; make the initialization constant with
`constexpr`/`constinit`; or the construct-on-first-use idiom — a function returning a
reference to a function-local `static`, which is initialized on first call and thread-safe
since C++11.

**13.** `PRIVATE` = used to build this target, not propagated. `INTERFACE` = not used to
build this target but propagated to consumers (header-only libraries). `PUBLIC` = both.
This is how you stop 40 downstream targets inheriting your internal include directories and
flags.

**14.** Because directory-based commands (`include_directories`, `add_definitions`) apply to
everything below them with no notion of what propagates, so flags leak, order matters, and
an installed package cannot carry its own usage requirements. Target-based CMake attaches
**usage requirements** to the target, so `target_link_libraries(app PRIVATE core)`
automatically gives `app` the right includes and definitions, and `install(EXPORT)`
reproduces them downstream.

**15.** Put every flag and definition on the **target** as `PUBLIC`/`INTERFACE` (or on a
shared `INTERFACE` "project_options" library that everything links), never per-file or via
global `add_definitions`. It matters because a macro that differs between two TUs compiling
the same header is an **ODR violation** — `NDEBUG`, a feature toggle, or
`_GLIBCXX_USE_CXX11_ABI` differing across TUs gives you silent heap corruption with a clean
link.

**16.** A single namespaced public include directory (`include/autonomy_core/…`) marked
`PUBLIC`, with `src/` marked `PRIVATE` so consumers physically cannot include internal
headers; `SOVERSION` for the ABI contract; `CXX_VISIBILITY_PRESET hidden` plus an explicit
export macro; only trivially copyable, standard-layout, fixed-width types in the public
headers with `static_assert`s pinning `sizeof`/`offsetof`; PIMPL for anything whose state
might grow; minimal public includes (forward declare, `std::span` rather than
`std::vector`); an installed `Config.cmake` via `install(EXPORT)`; and an **ABI diff check
in CI** (`abidiff`) that fails on an unintended change.

**17.** Many fast **unit** tests, fewer **integration** tests, fewest **end-to-end** tests.
Autonomy adds a fourth: **replay/regression on logged real-world data** — run the stack over
recorded drives and compare metrics against a baseline. That is what "integrate with our
evaluation pipeline" means.

**18.** **Write the failing test first, watch it fail, then fix it.** A test written after
the fix has never been observed to fail, so you have no evidence it covers the bug.

**19.** Because floating-point results depend on rounding, evaluation order, FMA
contraction, and optimization level, so an exact comparison fails for correct code
(`0.1 + 0.2 != 0.3`). Use `EXPECT_NEAR` with a tolerance derived from the computation's
error budget, or a mixed absolute+relative comparison.

**20.** It makes the test both slow and **flaky** — it passes on an idle laptop and fails on
a loaded CI machine, and the only fix anyone applies is a longer sleep. Instead: **inject
the clock** (a `FakeClock` template parameter), wait on the actual event with a latch,
future, or condition variable, or expose a test hook that signals completion. Examples in
`../../cpp-course/13-debugging-testing-and-sanitizers/examples.cpp` and
`../../leetcode/17-design-and-systems-questions/03-rate-limiter.cpp`.

**21.** So a failure is **reproducible**. An unseeded random test that fails once in CI gives
you nothing to debug and no way to verify the fix. Seed it with a constant, print the seed,
and let CI override it so you get both reproducibility and coverage.

**22.** Asserting a **property that holds for all inputs** rather than specific
input/output pairs — round-trip (`parse(serialize(x)) == x`), invariants (the result is
sorted), or equivalence to a slower reference implementation. Examples here: the parser
round-trip in `cpp-course/13`, the ring-buffer model test against a `std::deque`, and the
min/max queue checked against a brute-force scan.

**23.** Feeding automatically generated, coverage-guided inputs and checking the code does
not crash, hang, leak, or trip a sanitizer. Highest value for a parser because the input is
attacker- or sensor-controlled, the state space is huge, and humans are bad at guessing
malformed inputs — a fuzzer finds the truncated length field and the integer overflow in an
hour. Combine `-fsanitize=fuzzer,address,undefined`, keep a corpus in the repo, and minimize
every crash into a regression test.

**24.** **Line** coverage says which lines executed; **branch** coverage says which
*outcomes* of each condition did — much stronger, since `if (a && b)` can be 100%
line-covered with one of four paths taken. Coverage is a bad **target** (Goodhart): chasing
a number produces assertion-free tests. Use it to find untested code, and require coverage
of *changed* lines in review rather than a global percentage.

**25.** Measure first: run the suite 100+ times and rank tests by pass rate — usually three
tests cause 90% of the failures. **Quarantine with an owner and a deadline, do not
blanket-retry** (retries hide real regressions and remove the incentive to fix). Then fix by
category: sleeps → inject the clock; unseeded randomness → seed it; shared state → isolate
the fixture and run with `--gtest_shuffle`; shared ports/files → unique per test; real
network → fakes; genuine races → TSan. Make flakiness visible as a tracked metric, and
require new tests to survive `--gtest_repeat=50`. Treat a flaky test as a **bug in the
production code until proven otherwise** — often it is one.

**26.** **ASan**: heap/stack/global overflow, use-after-free/return, double free, leaks
(~2x). **UBSan**: signed overflow, bad shifts, misalignment, null deref, invalid enums
(~20%). **TSan**: data races, lock-order inversion (5-15x). **MSan**: uninitialized reads
(~3x, needs everything instrumented). **ASan and TSan cannot be combined.**

**27.** They are **dynamic** — they only see code paths that actually execute on the inputs
you provide. A bug on an untested branch is invisible. That is why they are paired with a
good test suite and fuzzing, and why static analysis is complementary rather than redundant.

**28.** `-D_GLIBCXX_ASSERTIONS`: libstdc++ precondition and bounds checks, a few percent.
`-D_FORTIFY_SOURCE=3` (with `-O1`+): checked `memcpy`/`strcpy`/`sprintf` using
compile-time-known sizes, near-zero cost. `-ftrivial-auto-var-init=zero`: initializes
otherwise-indeterminate locals for ~0-2%, converting a nondeterministic uninitialized-read
bug into a deterministic one. All three belong in production builds; none replaces a
sanitizer.

**29.** `bugprone-*` (use-after-move, dangling-handle, signed-char-misuse),
`performance-*` (unnecessary-value-param, for-range-copy, move-const-arg),
`cppcoreguidelines-*` (pro-type-reinterpret-cast, special-member-functions,
init-variables), `modernize-*` (use-override, make-unique, loop-convert), plus
`readability-*` and `concurrency-*`.

**30.** Fast lane on **every push** (the merge gate, target < 10 min): format and lint on
the diff; a build matrix (gcc + clang, Debug + RelWithDebInfo, plus the target
cross-compiler) with `-Werror`; unit tests; the same tests under ASan+UBSan; a handful of
microbenchmarks against a stored baseline. **On merge to main** (not blocking the author):
TSan on the concurrency tests, coverage of changed lines, integration tests, binary size and
build time as tracked metrics, and an ABI diff against the last release. **Nightly**:
fuzzing with a persistent corpus, the full replay evaluation over logged drives with metric
comparison, a 24-hour soak watching RSS and p99.9, and hardware-in-the-loop on the real SoC.
Two rules that make it work: the gate must stay **fast** or people batch merges around it,
and flaky tests get a quarantine with an expiry date rather than a retry.
