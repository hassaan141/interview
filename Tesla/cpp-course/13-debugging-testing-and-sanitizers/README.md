# 13 — Debugging, Sanitizers, Testing, CMake, Tooling

Course chapters: **17 (Debugging and Testing), 18 (Ecosystem)**

The posting says *"You will write tests and integrate with our evaluation pipeline to
ensure the system and functional stability of our stack."* This section is literally
in the job description.

---

## 1. Classify the defect before you debug it

| Kind | Symptom | Tool |
| --- | --- | --- |
| compile error | build fails | read the **first** error, not the last |
| link error | undefined/multiple symbol | `nm`, `c++filt`, see section 08 |
| logic error | wrong output, deterministic | debugger, unit test, bisect |
| memory error | crash, corruption, nondeterminism | ASan, valgrind |
| UB | works at -O0, breaks at -O2 | UBSan |
| race | nondeterministic, load-dependent | TSan |
| performance | too slow / misses deadline | `perf`, benchmarks |
| resource leak | grows over hours | LSan, `/proc/<pid>/status`, heaptrack |

The cost of a defect rises ~10x per phase (design → code → test → field), which is the
argument for sanitizers in CI rather than in a debugging session.

## 2. Assertions and contracts

```cpp
#include <cassert>
assert(index < size);                 // compiled out by -DNDEBUG
static_assert(sizeof(Frame) == 24);   // compile time, free

// A CHECK that stays on in release, for safety-relevant invariants:
#define CHECK(cond)                                                     \
    do { if (!(cond)) [[unlikely]] {                                    \
        std::fprintf(stderr, "CHECK failed: %s at %s:%d\n", #cond,      \
                     __FILE__, __LINE__);                              \
        std::abort();                                                   \
    } } while (0)

[[assume(x > 0)]];                    // C++23: tell the optimizer, do NOT check
std::unreachable();                   // C++23: UB if reached; enables better codegen
```
Opinion to have: **`assert` for programmer errors in test builds, a always-on `CHECK`
for invariants whose violation would be unsafe.** In a vehicle, a controlled abort
beats continuing with a corrupted state. Do not use `assert` for input validation —
inputs are runtime data and need real error handling.

`std::stacktrace` (C++23) gives you a portable backtrace; before that, `backtrace()` +
`abi::__cxa_demangle`, or `boost::stacktrace`.

## 3. Sanitizers — know what each one finds

```bash
# ASan + UBSan: the default for every test run. ~2x slower.
g++ -std=c++20 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined \
    -fno-sanitize-recover=all app.cpp -o app
ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:strict_string_checks=1 ./app

# TSan: data races and lock-order inversions. ~5-15x slower. Cannot mix with ASan.
g++ -O1 -g -fsanitize=thread -pthread app.cpp -o app
TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 ./app

# MSan (clang only, needs an instrumented libc++): uninitialized reads.
clang++ -fsanitize=memory -fsanitize-memory-track-origins=2 ...

# Cheap always-on hardening (no sanitizer, ~0-5% cost):
-D_GLIBCXX_ASSERTIONS            # libstdc++ bounds/precondition checks
-D_FORTIFY_SOURCE=3 -O2          # fortified memcpy/strcpy
-fstack-protector-strong -fstack-clash-protection
-fcf-protection=full             # control-flow integrity (CET)
-ftrivial-auto-var-init=zero     # kill uninitialized-read bugs at a tiny cost
```
| Sanitizer | Finds | Cost |
| --- | --- | --- |
| ASan | heap/stack/global OOB, UAF, use-after-return, double free, (+LSan) leaks | ~2x time, ~3x RAM |
| UBSan | signed overflow, misaligned access, bad casts, null deref, invalid enum/bool | ~20% |
| TSan | data races, lock-order inversion | 5-15x time, 5-10x RAM |
| MSan | reads of uninitialized memory | ~3x, needs full instrumentation |
| valgrind/memcheck | similar to ASan, no rebuild needed | 20-50x |

Sanitizers only find bugs on paths that **execute** — so they need good tests, stress
tests, and fuzzing. That is the connection to the next part.

## 4. Testing

```cpp
// GoogleTest
#include <gtest/gtest.h>
TEST(FrameParser, RejectsShortInput) {
    EXPECT_FALSE(parse({}).has_value());
}
TEST_F(ParserFixture, HandlesWrap) { /* uses SetUp/TearDown */ }
TEST_P(ParserParamTest, RoundTrips) { /* INSTANTIATE_TEST_SUITE_P */ }
ASSERT_EQ(a, b);   // fatal: aborts the test
EXPECT_EQ(a, b);   // non-fatal: continues, reports
EXPECT_NEAR(a, b, tol);      // ALWAYS for floating point
EXPECT_THROW(f(), std::out_of_range);
EXPECT_DEATH(f(), "CHECK failed");
ASSERT_THAT(v, ElementsAre(1, 2, 3));        // gmock matchers
MOCK_METHOD(int, read, (std::span<std::byte>), (override));
```
The test **pyramid**: many fast unit tests, fewer integration tests, fewest
end-to-end/simulation tests. For autonomy add a fourth layer: **replay/regression
tests on logged data**, which is what "integrate with our evaluation pipeline" means.

**Test-driven development**: red → green → refactor. Even if you do not do strict TDD,
write the failing test **before** the fix for every bug — that is the part that
actually prevents regressions, and it is the answer to give.

What makes a good test: one behaviour per test, a name that states the expectation,
deterministic (seed your RNG, inject the clock), no sleeps (use a fake clock or a
latch), fast (< 1 ms), and it must fail if you revert the fix. Test **boundaries**:
empty, one element, exactly capacity, capacity+1, max value, overflow, NaN, negative
zero.

**Code coverage**: `-fprofile-arcs -ftest-coverage` (gcov/lcov) or
`-fprofile-instr-generate -fcoverage-mapping` (clang). Use line **and branch**
coverage; treat it as a way to find untested code, not as a target to game.

**Fuzzing** — the highest-value testing technique for a parser:
```cpp
// libFuzzer: clang++ -fsanitize=fuzzer,address,undefined fuzz.cpp
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    (void)parse(std::span{data, size});     // must not crash, hang, or leak
    return 0;
}
```
Combine with ASan/UBSan so a memory error becomes a crash the fuzzer records. Add a
corpus and run it in CI. For a foundations team that owns wire-format parsing, this is
the single most effective thing you can say you would do.

## 5. GDB, in the commands you will actually use

```
gdb --args ./app arg1
break file.cpp:42            b main          tbreak (one-shot)
break f if x > 100           # conditional
watch counter                rwatch / awatch  # data breakpoints
catch throw                  # break on any exception
run / start / continue / next / step / finish / until
bt / bt full / frame 2 / up / down / info locals / info args
print expr / p *v._M_impl._M_start@v.size()   / p/x  / ptype
info registers / x/16xb ptr / disassemble /s
info threads / thread 3 / thread apply all bt
set var x = 5                # change state
reverse-continue             # with record, or rr
```
Know these three tools by name: **`rr`** (record and deterministically replay,
including reverse execution — the best tool in existence for a nondeterministic bug),
**`gdb` pretty printers** for STL types, and **core dumps**
(`ulimit -c unlimited`, `coredumpctl`, `gdb app core`).

## 6. Compiler warnings and static analysis

```bash
-Wall -Wextra -Wpedantic -Werror
-Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wcast-qual
-Wnon-virtual-dtor -Woverloaded-virtual -Wuseless-cast -Wdouble-promotion
-Wnull-dereference -Wduplicated-cond -Wlogical-op -Wformat=2
-Wimplicit-fallthrough -Wmisleading-indentation
-fanalyzer                       # gcc's static analyzer
clang++ -Weverything             # firehose, then disable what you disagree with
```
Turn on `-Werror` in CI, never locally-only. A warning you do not fix is a warning
everyone learns to ignore.

Static analysis: `clang-tidy` (with `bugprone-*`, `performance-*`,
`cppcoreguidelines-*`, `modernize-*`, `readability-*`), `cppcheck`, `include-what-you-
use`, gcc `-fanalyzer`, and commercially Coverity/PVS-Studio. `clang-format` for
formatting — settle it once in a `.clang-format` file and stop discussing it.

**Cyclomatic complexity** (independent paths through a function) and **cognitive
complexity** (how hard it is to read) are worth naming; `lizard` measures them. The
practical rule: a function over ~50 lines or complexity over ~10 wants splitting.

## 7. CMake — enough to be credible

```cmake
cmake_minimum_required(VERSION 3.20)
project(autonomy_core VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)      # for clang-tidy / IDEs

add_library(autonomy_core src/frame.cpp src/detector.cpp)
target_include_directories(autonomy_core
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE src)
target_compile_features(autonomy_core PUBLIC cxx_std_20)
target_compile_options(autonomy_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(autonomy_core PUBLIC Eigen3::Eigen PRIVATE fmt::fmt)

option(ENABLE_SANITIZERS "Build with ASan+UBSan" OFF)
if(ENABLE_SANITIZERS)
    target_compile_options(autonomy_core PUBLIC -fsanitize=address,undefined -g)
    target_link_options(autonomy_core PUBLIC -fsanitize=address,undefined)
endif()

include(CTest)
enable_testing()
add_executable(core_test tests/frame_test.cpp)
target_link_libraries(core_test PRIVATE autonomy_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(core_test)
```
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure -j
```
The two things to say: **target-based, not directory-based** (`target_*` commands,
never the global `include_directories`/`add_definitions`), and **`PUBLIC` /
`PRIVATE` / `INTERFACE` control what propagates to consumers** — which is how you stop
40 targets depending on your internal headers (and how you keep flags consistent, which
is the ODR defence from section 08).

Also name: `ninja` over `make`, `ccache`/`sccache`, `CMakePresets.json`,
`FetchContent`/`vcpkg`/`Conan` for dependencies, `add_custom_target(format)` wiring
`clang-format`, `doxygen` for API docs, and Compiler Explorer / quick-bench for
experiments.

---

## Traps checklist

1. Read the **first** compiler error; the rest are usually cascades.
2. "Works at -O0, breaks at -O2" means UB. Run UBSan.
3. ASan and TSan cannot be combined.
4. Sanitizers only see executed paths — they need tests and fuzzing.
5. `assert` is compiled out by `NDEBUG`; never put a side effect in one.
6. Never use `assert` for input validation.
7. `EXPECT_EQ` on floats is a broken test — use `EXPECT_NEAR`/`DoubleNear`.
8. A test with a `sleep` in it is a flaky test.
9. Write the failing test **before** the fix.
10. `-Werror` in CI or the warnings get ignored.
11. Target-based CMake, and `PUBLIC`/`PRIVATE` decide what leaks to consumers.
12. Coverage is a discovery tool, not a target.
