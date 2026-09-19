# 13 — Mock interview questions

The posting says you will "write tests and integrate with our evaluation pipeline".
C1 and C3 are the ones most likely to be asked in some form.

## A. Rapid fire

1. Program works at `-O0`, breaks at `-O2`. What is your first hypothesis and tool?
2. Which sanitizer finds what? Which two cannot be combined? Rough slowdowns?
3. What does ASan *not* find? What does UBSan not find?
4. What is the fundamental limitation of all sanitizers?
5. When is `assert` compiled out? Why must it never contain a side effect?
6. Why should you *not* use `assert` for input validation?
7. What is the case for a `CHECK` that stays on in release?
8. What do `-D_GLIBCXX_ASSERTIONS`, `-D_FORTIFY_SOURCE=3`, and
   `-ftrivial-auto-var-init=zero` buy you, and what do they cost?
9. `EXPECT_EQ` vs `ASSERT_EQ` in GoogleTest.
10. Why is `EXPECT_EQ(a, b)` on doubles a broken test?
11. Why is a `sleep()` in a test a bug? What do you do instead?
12. Why must a randomized test be seeded?
13. What is the test pyramid? What fourth layer does autonomy add?
14. What is your rule about writing tests when fixing a bug?
15. What is fuzzing, and why is it the highest-value technique for a parser?
16. Line coverage vs branch coverage. Why is coverage a bad target?
17. What is `rr` and when would you reach for it?
18. Name five gdb commands you use in a real session.
19. What is the difference between `PUBLIC`, `PRIVATE`, and `INTERFACE` in CMake?
20. Why is target-based CMake better than the directory-based style?
21. Why `-Werror` in CI but not locally?
22. Name four clang-tidy check families and what each catches.
23. What is cyclomatic complexity, and what threshold do you act on?

## B. Diagnose

**B1.** A test passes locally and fails in CI, roughly one run in twenty.

**B2.** A release build crashes; the debug build is fine. `gdb` on the core shows a
corrupted stack with no useful frames.

**B3.** A service leaks ~2 MB/hour. Valgrind reports "still reachable, no leaks".

**B4.** After enabling `-O3`, a numerical regression test's output changes in the 12th
decimal place.

**B5.** A unit test suite takes 40 minutes; most of it is one integration test that
starts a real network server.

**B6.** ASan reports `heap-use-after-free` with a free stack inside
`std::vector<T>::push_back`.

**B7.** Your fuzzer finds a crash after 3 hours. The input is 4 KB of random bytes.

**B8.** A `CHECK` fires in the field once a week; you cannot reproduce it.

## C. Whiteboard

**C1.** Design the CI pipeline for `autonomy_core`. What jobs, in what order, with
what gates, and what do you do about the ones that are too slow to run on every
commit?

**C2.** You own the wire-format parser that every autonomy process uses. Write the
test plan.

**C3.** "Integrate with our evaluation pipeline to ensure the system and functional
stability of our stack." What does that mean concretely, and how would you build it?

**C4.** Make a flaky test suite trustworthy again. Give the process.

---
---

# Answers

**A1.** **Undefined behavior.** At `-O0` the generated code mirrors the source, so UB
often does the "obvious" thing; at `-O2` the optimizer assumes UB cannot happen and
deletes or reorders code accordingly. First tool: rebuild with
`-fsanitize=undefined,address -fno-sanitize-recover=all` and run the failing case.
Second: `-Wall -Wextra -Wmaybe-uninitialized` and gcc `-fanalyzer`. Third: bisect the
optimization flags (`-O2 -fno-strict-aliasing`, `-fwrapv`) — if `-fwrapv` fixes it,
you have signed overflow; if `-fno-strict-aliasing` fixes it, you have an aliasing
violation.

**A2.** **ASan**: heap/stack/global out-of-bounds, use-after-free,
use-after-return/scope, double free, and with LSan, leaks — ~2x time, ~3x memory.
**UBSan**: signed overflow, shifts past width, misaligned access, null dereference,
invalid enum/bool values, bad `dynamic_cast` — ~20%. **TSan**: data races,
lock-order inversion — 5-15x time, 5-10x memory. **MSan** (clang): reads of
uninitialized memory — ~3x, and it needs *every* library instrumented.
**ASan and TSan cannot be combined.**

**A3.** ASan does not find: uninitialized reads (that is MSan), data races (TSan),
integer overflow or other pure-UB (UBSan), logic errors, or overflows *within* a
struct's own allocation (it only guards the redzones around allocations). UBSan does
not find memory-safety bugs, leaks, or races. None of them find design errors.

**A4.** They are **dynamic** — they only observe code paths that actually execute on
the inputs you give them. A bug on an untested branch is invisible. That is why
sanitizers are paired with a good test suite, stress tests, and **fuzzing**, and why
static analysis (`clang-tidy`, `-fanalyzer`, Coverity) is complementary rather than
redundant.

**A5.** When `NDEBUG` is defined — which every `Release` CMake build does by default.
The whole expression disappears, so `assert(v.pop() == 3)` silently stops popping in
release. Never put a side effect, a state change, or a function call you need in an
`assert`.

**A6.** Because input is *runtime data*, not a programmer error: bad input from a
sensor, a file, or a network peer is an expected condition that must be handled and
reported, not a bug to abort on — and the `assert` disappears in release exactly where
the hostile input arrives. `assert` is for invariants you believe are impossible;
validation is for data you do not control.

**A7.** In a safety-relevant system, continuing with a violated invariant is worse
than stopping. An always-on `CHECK` turns "silent corruption propagating into a
control output" into "a deterministic, logged, reproducible abort with a known state",
which the supervisor can handle by degrading gracefully. The cost is a predictable
branch (use `[[unlikely]]`) which is essentially free. Use `assert` for the cheap
paranoid checks and `CHECK` for the ones whose failure is unsafe.

**A8.** `-D_GLIBCXX_ASSERTIONS` enables libstdc++ precondition checks (`vector::
operator[]` bounds, invalid iterator ranges) for a few percent. `-D_FORTIFY_SOURCE=3`
(with `-O1`+) replaces `memcpy`/`strcpy`/`sprintf` with checked variants using
compile-time-known sizes, near-zero cost. `-ftrivial-auto-var-init=zero` initializes
otherwise-indeterminate locals, costing ~0-2% and converting a nondeterministic
uninitialized-read bug into a deterministic (and usually benign) one. All three are
worth enabling in production builds; they are not sanitizers and do not replace them.

**A9.** `EXPECT_*` reports the failure and **continues** the test function;
`ASSERT_*` reports and **returns immediately** from the test. Use `ASSERT` when
continuing would crash or produce cascading noise (e.g. after checking that an
`optional` has a value, before dereferencing it), `EXPECT` otherwise so one run
reports all the failures.

**A10.** Because floating-point results depend on rounding, evaluation order, FMA
contraction, and the optimization level, so an exact comparison fails for correct code
(`0.1 + 0.2 != 0.3`) — the test itself becomes the bug. Use `EXPECT_NEAR(a, b, tol)`,
`EXPECT_DOUBLE_EQ` (which is ULP-based), or a mixed absolute+relative tolerance
(section 01), and justify the tolerance from the computation's error budget.

**A11.** Because it makes the test both **slow** and **flaky**: it passes on an
unloaded laptop and fails on a loaded CI machine, and the only "fix" anyone applies is
to increase the sleep, which makes the suite slower without making it correct.
Instead: **inject the clock** (a `FakeClock` template parameter, as in
`examples.cpp`), use a `std::latch`/`condition_variable`/future to wait on the actual
event, or expose a test hook that signals completion.

**A12.** So a failure is **reproducible**. An unseeded random test that fails once in
CI gives you nothing to debug and cannot be verified as fixed. Seed it with a fixed
constant, print the seed, and (best practice) allow overriding the seed from the
environment so CI can also run randomized sweeps while any individual failure is
replayable.

**A13.** Many fast **unit** tests (milliseconds, no I/O), fewer **integration** tests
(a few components together), fewest **end-to-end** tests (the whole system). Autonomy
adds a fourth: **replay / regression on logged real-world data** — run the stack over
recorded drives and compare metrics against a baseline. That is the layer the job
posting calls the "evaluation pipeline".

**A14.** **Write the failing test first, watch it fail, then fix it.** A test written
after the fix has never been observed to fail, so you have no evidence it covers the
bug. This is the single highest-value testing habit and the answer to give.

**A15.** Feeding automatically generated (usually coverage-guided, mutation-based)
inputs to a function and checking it does not crash, hang, leak, or trip a sanitizer.
It is the highest-value technique for a parser because a parser's input is attacker-
or sensor-controlled, its state space is enormous, and humans are bad at guessing
malformed inputs — a fuzzer finds the truncated length field, the 0-length payload,
and the integer overflow in an hour. Combine `-fsanitize=fuzzer,address,undefined`,
keep a corpus in the repo, and run it both in CI (short) and continuously (long).

**A16.** **Line coverage** = which lines executed. **Branch coverage** = which
*outcomes* of each condition executed — much stronger, since a line with `if (a && b)`
can be 100% line-covered with only one of four paths taken. Coverage is a bad
**target** (Goodhart): chasing a number produces assertion-free tests that execute code
without checking anything. Use it to **find untested code**, and require coverage of
*new* code in review rather than a global percentage.

**A17.** `rr` records a program's execution and replays it **deterministically**,
including reverse execution (`reverse-continue`, `reverse-step`) in gdb. Reach for it
for any bug that is nondeterministic, rare, or where you need to run *backwards* from a
corrupted value to the write that caused it — heisenbugs, races (once recorded, the
replay is deterministic), and "who freed this?" It is the single biggest debugging
productivity tool on Linux.

**A18.** `break file:line if cond` (conditional breakpoint), `watch expr` (data
breakpoint — the fastest way to find who corrupts a value), `bt full` /
`thread apply all bt` (all threads' stacks, the first thing to do on a deadlock core),
`finish` / `until`, `p` and `x/16xb` for memory, `info locals`, `catch throw`,
`set var` to test a hypothesis without recompiling.

**A19.** `PRIVATE` = used when building this target only, not propagated to consumers.
`INTERFACE` = not used building this target, but propagated to consumers (for
header-only libraries). `PUBLIC` = both. Getting this right is what stops 40 downstream
targets from inheriting your internal include directories and flags — and, since it
controls the definitions every consumer compiles with, it is also the ODR defence from
section 08.

**A20.** Because directory-based commands (`include_directories`, `add_definitions`,
`link_libraries`) apply to *everything* in the directory and below, with no notion of
what propagates to consumers — so flags leak, targets become order-dependent, and an
installed package cannot carry its own usage requirements. Target-based CMake attaches
**usage requirements** to the target, so `target_link_libraries(app PRIVATE
autonomy_core)` automatically gives `app` the right includes, definitions, and
features, and `install(EXPORT ...)` reproduces them for downstream projects.

**A21.** Because a warning that does not break the build gets ignored, and a codebase
with 3000 warnings has zero useful warnings. `-Werror` in CI makes the ratchet
one-directional. Not locally (or at least, easy to disable locally) because a
developer mid-refactor with an unused variable should not be blocked, and because a new
compiler version can introduce warnings that would break everyone's local build on
upgrade day.

**A22.** `bugprone-*` (likely bugs: `use-after-move`, `dangling-handle`,
`signed-char-misuse`), `performance-*` (`unnecessary-value-param`,
`for-range-copy`, `move-const-arg`), `cppcoreguidelines-*` (`pro-type-reinterpret-
cast`, `special-member-functions`, `init-variables`), `modernize-*` (`use-nullptr`,
`use-override`, `make-unique`, `loop-convert`), `readability-*`,
`concurrency-*`, and `misc-*`. Run with `--fix` for the mechanical ones, and gate new
code on the `bugprone`/`performance` families.

**A23.** The number of linearly independent paths through a function (roughly, one plus
the number of decision points). It correlates with the number of tests needed for full
branch coverage and with defect density. Practical thresholds: over ~10 warrants a
look, over ~20 is a refactor. Measure with `lizard` or `clang-tidy
readability-function-cognitive-complexity`. Cognitive complexity is the better metric
because it penalizes nesting, which is what actually makes code hard to read.

---

**B1.** A **flaky test** — and one-in-twenty is the signature of a race, a timing
dependency, or shared state. Process: (1) do not retry it away; quarantine it with a
ticket so it stops blocking, but fix it. (2) Reproduce: run it 1000 times in a loop
locally, and under `--gtest_shuffle` (test ordering dependency) and `-j` (parallel
interference). (3) If it involves threads, run it under **TSan**. (4) Look for the
usual causes: a `sleep`-based synchronization, an unseeded RNG, dependence on wall
time or the filesystem, a shared temp file/port, a static that leaks state between
tests, or hash-map iteration order. (5) Fix the *cause* — inject the clock, use a
latch, seed the RNG, isolate the fixture. CI is more likely to catch it because it is
slower, more contended, and differently scheduled.

**B2.** Almost certainly **UB or memory corruption** — a stack buffer overflow smashing
the return address is exactly "corrupted stack, no frames". Steps: rebuild the
*release* configuration with `-fsanitize=address,undefined -g
-fno-omit-frame-pointer` (keep `-O2` so the optimization-dependent bug survives) and
run the failing input; add `-fstack-protector-strong` which converts the smash into a
clean `__stack_chk_fail` abort; use `rr` to record and run backwards from the crash;
check `-Wall -Wextra` output you have been ignoring. Also verify the release build is
not tripping over an `assert` that was carrying a side effect, and that `NDEBUG` is
set consistently in every TU (section 08).

**B3.** "Still reachable" means the memory is **not** leaked in valgrind's sense — a
live pointer still exists — so this is unbounded **growth**, not a leak: a cache with
no eviction policy, a vector you only ever `push_back` to, a log/metrics buffer, a
`shared_ptr` cycle (which *is* still reachable from itself), or an ever-growing map
keyed by something unbounded like a request ID. Tools: **heaptrack** or
`massif` (shows allocation growth by call site over time), `jemalloc`/`tcmalloc` heap
profiles, or periodically dumping container sizes. Fix: bound every cache (LRU with a
size cap), and add a monitored metric for the container sizes so the next one is caught
in an hour rather than a week.

**B4.** Expected and almost certainly **not a bug**: `-O3` enables vectorization and
reassociation-adjacent transforms, and `-ffp-contract=fast` (on by default) lets the
compiler fuse `a*b+c` into an FMA, which has *different* (in fact more accurate)
rounding. A vectorized reduction also sums in a different order, and FP addition is not
associative (section 01). Response: set the test's tolerance from the computation's
error budget rather than bit equality; if you genuinely need bit-reproducibility (log
replay!), compile the numerical core with `-ffp-contract=off` and a fixed-shape
deterministic reduction, and document that requirement. What you must *not* do is
"fix" it by pinning `-O2`.

**B5.** The pyramid is upside-down. Fix: (1) move the network server behind an
interface and **inject a fake** in the unit tests — the server is not what you are
testing; (2) keep one real end-to-end test, and run it in a **separate CI stage**
(nightly or pre-merge, not per-commit); (3) parallelize (`ctest -j`,
`gtest_discover_tests` so each case is its own CTest entry); (4) add a test-time budget
to CI so a new slow test is caught when it is added. Target: the unit suite under a
minute, because a suite nobody runs locally stops catching anything.

**B6.** The free came from `std::vector`'s **reallocation**: a pointer, reference, or
iterator into the vector was captured, then a `push_back` grew it, freeing the old
buffer — and the code kept using the stale pointer. ASan's "freed by thread T here"
stack pointing at `push_back` is the giveaway. Fix: re-acquire after any operation that
can reallocate, `reserve()` up front, store an **index** instead of a pointer, or use a
container with stable references (`deque` for end-insertion, `list`, or a node-based
map). This is the single most common ASan report in C++ code.

**B7.** (1) **Save and minimize the input** — libFuzzer's `-minimize_crash=1` (or
`afl-tmin`) will usually shrink 4 KB to a handful of bytes that make the bug obvious.
(2) Commit the minimized input as a **regression test** in the corpus, so it runs
forever after. (3) Fix the bug, then re-run the fuzzer on the same corpus to confirm
and to look for neighbours. (4) Ask *why the class of bug was possible* — if it was an
unchecked length field, audit every other length field. (5) Keep the corpus in the repo
and run a short fuzz job in CI plus a long one continuously (OSS-Fuzz style), since
fuzzing finds more the longer it runs.

**B8.** You need the state at the moment of failure, not a reproduction. (1) Make the
`CHECK` capture context: the values involved, a `std::stacktrace`, the last N entries
of an in-memory **ring-buffer trace** that is only dumped on failure — this is the
technique that works for rare events, because you instrument always and keep only the
tail. (2) Ensure core dumps are collected and symbolized (`coredumpctl`, build-id
matching, keep the unstripped binaries). (3) Log the inputs: with a replayable log
format you can re-run the exact drive offline, which is precisely what the evaluation
pipeline is for. (4) Statistically correlate the occurrences — same vehicle? same
sensor? same time of day? cold start? (5) Meanwhile, add the invariant check *earlier*
so the failure is caught closer to the cause, and consider a targeted sanitizer build
deployed to a small fleet subset.

---

**C1.**
```
On every push (must be fast — target < 10 min, this is the merge gate):
  1. format + lint      clang-format --dry-run --Werror; clang-tidy on the DIFF only
  2. build matrix       gcc + clang, Debug + RelWithDebInfo, -Werror
                        (and the target's cross-compiler, at least a compile check)
  3. unit tests         ctest -j, must be < 2 min
  4. ASan + UBSan       unit tests again, instrumented
  5. quick benchmarks   a handful of microbenchmarks vs. a stored baseline,
                        fail only on a large regression (> 10%)

On merge to main (10-40 min, not blocking the author):
  6. TSan               the concurrency-touching tests
  7. coverage           report, and gate on coverage of CHANGED lines
  8. integration tests  multi-component, still no real hardware
  9. binary size + build time   tracked as metrics with a budget
 10. ABI check          abidiff against the last release for the public library

Nightly / continuous (hours):
 11. fuzzing            all parsers, persistent corpus, minimized crashes filed
 12. replay evaluation  the full stack over a corpus of logged drives, metrics
                        compared against the baseline (the "evaluation pipeline")
 13. long soak          24 h run watching RSS, fd count, p99.9 latency
 14. HIL / vehicle      hardware-in-the-loop on the real SoC
```
The parts to defend out loud:
- **The gate must stay fast** or people stop running it and start batching merges.
  Everything slow moves to post-merge with an automatic revert/bisect on failure.
- **`-Werror` and the format check are non-negotiable and cheap** — they are the
  ratchet that keeps the other signals meaningful.
- **Flaky tests are bugs**: a quarantine list with an owner and an expiry date, and a
  build that fails if a test has been quarantined for more than N days. Never a blanket
  retry.
- **Track trends, not just pass/fail**: build time, binary size, benchmark results, and
  p99 latency as time series, because those regress gradually and no single commit
  looks bad.
- Cross-compile for the **target architecture** in the fast lane; ARM and x86 differ in
  alignment, char signedness, and memory ordering (section 11), so an x86-only CI is a
  false sense of safety.

**C2.** Test plan for a parser every process depends on:
1. **Compile-time**: `static_assert` on `sizeof`/`alignof`/`offsetof` of every wire
   struct, on trivial copyability and standard layout, and on the enum underlying
   types. A layout change must break the build, not the fleet.
2. **Unit tests, boundary-first**: empty input; one byte below the minimum; exactly the
   minimum; exactly the maximum payload; one above the maximum; a length field larger
   than the buffer (the hostile case); a truncated payload; a corrupted checksum; all
   reserved/unknown IDs; every error enum reachable. See `examples.cpp` — those are
   exactly these cases.
3. **Round-trip property test**: for any valid frame, `parse(serialize(f)) == f`, over
   thousands of seeded random frames. This catches the asymmetric-encoding bugs unit
   tests miss.
4. **Fuzzing**: `LLVMFuzzerTestOneInput` over `parse`, built with
   `-fsanitize=fuzzer,address,undefined`, with a committed corpus seeded from real
   captures, run short in CI and continuously out of band. Every crash gets minimized
   and committed as a regression case.
5. **Differential testing** where possible: compare against the previous version of
   the parser, or against an independently written reference (a Python implementation),
   on the same corpus. Any divergence is a bug in one of them.
6. **Real-data replay**: run the parser over hours of logged bus traffic and assert no
   errors, no allocations, and a bounded worst-case parse time.
7. **Negative/robustness contract, asserted explicitly**: `noexcept`, no allocation
   (test with an allocation-counting global `operator new`), bounded execution time,
   and total memory safety on arbitrary input.
8. **Performance regression**: a microbenchmark on a fixed corpus with a stored
   baseline, because this code is on the hot path of every process.
Then the meta-point: this is a component where a bug is remotely triggerable by
malformed bus data, so the bar is memory safety on *all* inputs, not just correctness
on valid ones.

**C3.** Concretely, an evaluation pipeline for an autonomy stack is:
- **A corpus of logged drives** (and simulated scenarios), versioned and immutable, with
  a scenario taxonomy — highway, urban, night, rain, construction, rare events.
- **Deterministic replay**: feed recorded sensor data through the real stack and get
  bit-identical results for the same build. That requirement is what forces fixed-shape
  FP reductions, a seeded RNG, an injected clock, and no dependence on thread
  scheduling — and it is the single hardest engineering property to maintain. It is
  also exactly where a *foundations* engineer contributes.
- **Metrics per run**, both functional and system:
  *functional* — detection precision/recall, tracking ID switches, planner comfort and
  intervention counts, collision/near-miss in sim;
  *system* — per-stage latency p50/p99/p99.9, end-to-end sensor→actuation latency,
  frame drops, CPU/NPU utilization, memory high-water mark, allocation counts,
  thermal headroom.
- **Baselines and gates**: every candidate build is compared to the current production
  build on the same corpus. Functional metrics gate on statistically significant
  regressions; system metrics gate on absolute budgets (e.g. "p99 end-to-end < 100 ms
  on the target SoC"). Report both the aggregate and the **worst scenarios**, because a
  mean hides the safety-relevant tail.
- **Infrastructure**: distributed runners (replaying thousands of drives is embarrassingly
  parallel), artifact storage for per-run traces, a dashboard with time series, and
  automatic bisection when a metric regresses.
- **Hardware in the loop**: the final stage runs on the actual FSD hardware, because
  latency and throughput numbers from an x86 workstation do not transfer.

What I would build as a foundations engineer specifically: the **instrumentation and
tooling layer** — a low-overhead tracing/timing API every stage uses (a scoped
`TRACE_SPAN` writing into a per-thread lock-free ring buffer, flushed off the hot path),
the harness that runs a build over the corpus and emits structured metrics, the
regression detector, and the determinism checks that make replay meaningful. That is
almost word-for-word what the posting describes.

**C4.** Process for making a flaky suite trustworthy:
1. **Measure it.** Run the whole suite 100+ times and record per-test pass rates. You
   cannot fix what you cannot rank, and usually 3 tests cause 90% of the failures.
2. **Quarantine, do not retry.** Move the flaky tests to a non-blocking job with an
   owner and a deadline. A blanket "retry 3 times" is how a suite dies: it hides real
   regressions and removes the incentive to fix anything.
3. **Fix causes, by category**: `sleep`-based synchronization → inject the clock or use
   a latch; unseeded randomness → seed and print it; shared state between tests →
   isolate the fixture, run with `--gtest_shuffle` to prove independence; shared ports,
   temp files, or a database → unique per-test resources; real network/filesystem →
   fakes; genuine data races → TSan.
4. **Make flakiness visible and expensive**: dashboard per-test pass rate, fail the
   build if a test's flake rate exceeds a threshold, and auto-file a ticket. Treat a
   flaky test as a **bug in the production code until proven otherwise** — quite often
   it is one (a real race the test happened to expose).
5. **Prevent regression**: require new tests to pass a repeat-run (`--gtest_repeat=50`)
   in CI when they are added, ban `sleep` via a lint rule, and require any test touching
   threads to have a TSan job.
6. Close the loop by reporting the suite's flake rate as a tracked metric, because
   "trustworthy" is a property you have to keep, not one you achieve once.
