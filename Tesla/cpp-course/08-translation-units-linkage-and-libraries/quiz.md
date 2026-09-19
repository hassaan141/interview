# 08 — Mock interview questions

## A. Rapid fire

1. What is a translation unit, exactly?
2. Name the three kinds of linkage, plus the C++20 one.
3. Give the four unrelated meanings of `static`.
4. What is the linkage of `const int kMax = 10;` at namespace scope in C++? In C?
5. What does `inline` actually mean? What does it *not* mean?
6. What did C++17 `inline` variables fix?
7. State the One Definition Rule in three parts.
8. What is IFNDR and why does it make ODR violations so dangerous?
9. Give a concrete way two TUs end up with different layouts for the same class.
10. What is the static initialization order fiasco? Give three fixes.
11. Are function-local statics thread-safe? Since when? What does it cost?
12. What is `constinit` for? How does it differ from `constexpr`?
13. `thread_local` — storage duration, when is it initialized, what does it cost?
14. Why does `undefined reference to vtable for X` happen?
15. Why does a template defined in a `.cpp` fail to link from another TU? Two fixes.
16. What is `extern "C"` for, and why can't you overload such a function?
17. Why does C++ mangle names at all?
18. Difference between `#include <x>` and `#include "x"`.
19. Two reasons to forward declare instead of including.
20. Static vs. shared library: four trade-offs.
21. Why does static-library link order matter?
22. Define ABI. Name five changes that break it.
23. What does `-fvisibility=hidden` do and why would you want it?
24. What problem do C++20 modules solve? Name three module-unit kinds.

## B. Diagnose the error

**B1.** `multiple definition of 'helper()'` — header contains `int helper() { return 1; }`.

**B2.** `undefined reference to 'Config::instance()'` — it's declared `static` in the class.

**B3.**
```cpp
// util.hpp
static std::vector<int> g_cache;
// used from 5 .cpp files; each one seems to get a different cache
```

**B4.**
```cpp
// vec.hpp
template <typename T> class Vec { void grow(); };
// vec.cpp
template <typename T> void Vec<T>::grow() { /* ... */ }
// main.cpp
Vec<int> v; v.grow();        // undefined reference
```

**B5.**
```cpp
// logger.cpp
Logger g_logger;                     // needs g_config
// config.cpp
Config g_config{"/etc/app.conf"};
// crashes at startup, sometimes
```

**B6.**
```cpp
// a.cpp compiled with -DNDEBUG ; b.cpp compiled without
// header:
struct Tracker {
    int id;
#ifndef NDEBUG
    std::string debug_name;
#endif
};
```

**B7.**
```cpp
// plugin v1.0 shipped this in a header:
class Detector {
    int threshold_;
public:
    virtual ~Detector();
    virtual void detect();
};
// v1.1 adds a member and a virtual, keeps the same .so name
class Detector {
    int threshold_;
    float scale_;
public:
    virtual ~Detector();
    virtual void reset();
    virtual void detect();
};
```

**B8.**
```cpp
extern "C" int process(const char* s);
extern "C" int process(const char* s, int len);
```

## C. Whiteboard

**C1.** Design the file/CMake layout for a foundations library
`autonomy_core` that is consumed by 40 application targets, some of which are
built as separately shipped shared objects. State how you keep the ABI stable and
the build fast.

**C2.** A 500-TU project takes 40 minutes to build and 6 minutes to link. Give an
ordered plan with the expected win from each step.

**C3.** You are told "the same binary gives different results depending on link
order". Walk through your diagnosis.

---
---

# Answers

**A1.** One source file **after preprocessing** — the `.cpp` plus every header it
transitively includes, with macros expanded and `#if` branches resolved. It is the
unit the compiler sees; the compiler knows nothing about any other TU, and the
linker knows nothing about types.

**A2.** No linkage (locals, parameters), internal linkage (visible only in this TU:
`static` at namespace scope, anything in an anonymous namespace, namespace-scope
`const`/`constexpr`, typedefs, enums), external linkage (visible program-wide),
and C++20 **module linkage** (visible only within the module).

**A3.** (1) At namespace scope: internal linkage. (2) At block scope: static storage
duration — one object, initialized on first pass, destroyed at exit. (3) A class
data member: one per class rather than per object. (4) A class member function: no
`this`, callable as `C::f()`.

**A4.** In **C++** it has **internal** linkage (implicitly, because the standard
makes namespace-scope `const` internal so it can appear in headers). In **C** it has
**external** linkage, which is why the same header in C needs `static` or `extern`
discipline. Add `extern` in C++ to give it external linkage.

**A5.** It means: **this entity may be defined in more than one translation unit,
and the linker must merge the definitions into one** (implemented as a weak/COMDAT
symbol). It is what makes a function definition legal in a header. It does **not**
mean "expand this call at the call site" — that is entirely the optimizer's
decision, driven by cost heuristics, `always_inline`, and `-O` level. `constexpr`
functions are implicitly `inline`.

**A6.** Header-only definitions of **variables**. Before C++17 a namespace-scope
variable needed a `.cpp` with the one definition (`int Foo::count_ = 0;`), or you
used the function-local-static workaround. `inline int g = 0;` in a header now gives
exactly one object across the program.

**A7.** (1) At most one definition per TU of any entity. (2) Exactly one definition
in the whole program of every non-`inline` function or variable that is *used*.
(3) Classes, `inline` functions/variables, templates, `constexpr`, and enums may be
defined in multiple TUs **provided every definition consists of the same token
sequence and those tokens mean the same thing in every TU**.

**A8.** *Ill-Formed, No Diagnostic Required*: the program is invalid but no
implementation is required to tell you. For ODR violations the linker simply keeps
one of the definitions and discards the others, so half the program uses one layout
or one function body and the other half uses a different one. There is no error, no
warning, and the resulting corruption appears far from the cause. `-flto` plus
`-Wodr` is the practical detector.

**A9.** Compiling two TUs with different `-D` flags (`NDEBUG`, a feature toggle),
different `-std=`, different `#pragma pack`, or including the header after
different other headers so a macro changes it. Also: an `inline` function whose body
differs because it refers to something from an anonymous namespace, or two
third-party libraries that each define `class Logger` at global scope.

**A10.** Namespace-scope objects requiring **dynamic** initialization are
initialized in unspecified order **across** TUs, so one global can be used before it
is constructed. Fixes: (1) remove the mutable global; (2) make the initialization
*constant* with `constexpr`/`constinit` so it happens before any code runs;
(3) the construct-on-first-use idiom — a function returning a reference to a
function-local `static`, which is initialized on the first call and is thread-safe
since C++11. Mention the mirror problem (destruction order → the *de*initialization
fiasco) and the nifty-counter idiom.

**A11.** Yes, since **C++11**: the implementation must guarantee that exactly one
thread runs the initializer and the others block until it completes (`__cxa_guard_
acquire` on the Itanium ABI). Cost: one relaxed atomic load of a guard byte on every
subsequent call — usually free after inlining, but it is not literally zero, which
matters in a 1 kHz loop. `-fno-threadsafe-statics` removes it if you can prove
single-threaded initialization.

**A12.** `constinit` asserts that a variable with static or thread storage duration
is **constant-initialized**, so it cannot participate in the ordering fiasco — but
the object remains **mutable** afterwards. `constexpr` also implies `const`. So
`constinit` is for "a global I must be able to write, but whose initialization must
not run at startup".

**A13.** Thread storage duration: one object per thread, destroyed when that thread
exits. Initialized before first use in that thread (dynamic initialization is
effectively lazy, guarded). Cost: access goes through the thread pointer plus a TLS
offset — cheap in the initial-exec model, but a `__tls_get_addr` call in the general
dynamic model used by shared libraries, which is meaningfully slower, and each
thread pays the memory.

**A14.** The compiler emits a class's vtable in the TU that defines its **key
function** — the first non-inline, non-pure virtual function in declaration order,
in practice usually the destructor. If every virtual is declared but none is
defined, no TU emits the vtable. Fix: define at least one virtual out of line (the
common idiom is to define the destructor in the `.cpp`, which also gives you one
canonical place for the vtable and improves build times).

**A15.** Because the compiler instantiates a template where it is *used*, and it can
only do so if the **definition** is visible in that TU. Fixes: (1) put the
definition in the header (or a `-inl.h`/`.tpp` included at the end of it);
(2) **explicit instantiation** in the `.cpp` (`template class Vec<int>;`) plus
`extern template class Vec<int>;` in the header — which also cuts build time when
the set of instantiations is small and known.

**A16.** It gives a function **C language linkage**: the symbol is not mangled and
the C calling convention is used, so C code (and any other language's FFI, and a
different C++ compiler) can call it. You cannot overload it because overloading is
implemented *by mangling* — two overloads would need to produce the same unmangled
symbol name.

**A17.** Because C++ has overloading, namespaces, classes, templates, and cv/ref
qualifiers, all of which can produce many distinct entities with the same
identifier. Mangling encodes the full signature into a unique linker symbol. It also
makes the ABI type-safe-ish: linking a TU that expects `f(int)` against one that
defines `f(double)` fails at link time instead of corrupting the stack. `c++filt`
and `nm -C` demangle.

**A18.** `"x"` searches relative to the directory of the *including file* first, then
falls back to the `<>` search path. `<x>` searches only the implementation-defined
path list (`-I`, `-isystem`, and the built-in system directories). Convention: `<>`
for the standard library and third-party dependencies, `""` for your own project.
`-isystem` additionally suppresses warnings from those headers.

**A19.** (1) **Build time** — a forward declaration costs nothing, while an
`#include` drags the whole transitive graph into every TU that includes yours, and
changes to that header recompile everything. (2) **Breaking dependency cycles**,
which cannot be done with includes. (3) Bonus: it reduces the surface for ODR
violations and keeps your header's ABI from depending on someone else's layout.

**A20.** (a) **Size/deployment**: static duplicates code into every consumer but
ships one file; shared is one copy on disk, shared across processes in the page
cache, but must be found at load time. (b) **Update**: shared can be swapped
without relinking *if the ABI holds*; static requires a relink. (c) **Performance**:
static allows cross-module inlining and whole-program LTO and has no PLT/GOT
indirection; shared pays an indirect call per cross-library call and cannot inline
across the boundary. (d) **Determinism/startup**: static has no loader symbol
resolution and no `LD_LIBRARY_PATH` surprises; shared has faster link times during
development. For an automotive/embedded target, static + LTO is usually the right
default.

**A21.** A static archive is just a bag of `.o` files, and the traditional linker
processes its inputs **left to right**, pulling an object out of an archive only if
it resolves a symbol that is *currently* undefined. If the archive appears before
the object that needs it, nothing is undefined yet, nothing is extracted, and you
get an undefined reference. Hence: objects first, libraries last, and
`-Wl,--start-group ... --end-group` for mutually dependent archives.

**A22.** The Application Binary Interface: everything two separately compiled
binaries must agree on — name mangling, struct/class layout and padding, alignment,
calling convention and register usage, vtable layout and offsets, RTTI
representation, exception unwinding tables, and the standard library's own type
layouts. Breaking changes: adding/removing/reordering **virtual** functions; adding,
removing, or reordering **data members**; changing a type's size or alignment;
changing a function's signature or return type; changing the body of an `inline`
function that other binaries already inlined; changing a default argument; changing
an enum's underlying type; toggling `-D_GLIBCXX_USE_CXX11_ABI`.

**A23.** It makes all symbols hidden (not exported from the shared object) unless
explicitly marked `__attribute__((visibility("default")))`. Benefits: a much smaller
dynamic symbol table (faster load, smaller binary), the linker and LTO can
devirtualize and inline aggressively because they know nothing outside can override,
no accidental ABI surface, and no symbol collisions between plugins. It is the
standard practice for any shipped `.so`.

**A24.** Modules solve: repeated parsing of the same headers in every TU (build
time), macro leakage across boundaries, implicit interfaces (everything in a header
is public), include-order dependence, and most ODR hazards. Unit kinds: **module
interface unit** (`export module M;`), **module implementation unit**
(`module M;`), **module partition** (`export module M:part;`), plus the **global
module fragment** (`module;` followed by `#include`s) and the **private module
fragment** (`module :private;`). Header units (`import <vector>;`) bridge to legacy
headers.

---

**B1.** A non-`inline` function **definition** in a header gets compiled into every
TU that includes it, so the linker sees N definitions of an external-linkage symbol.
Fix: mark it `inline` (or `constexpr`, or `static`/anonymous-namespace if it really
is TU-local, or move the definition to one `.cpp`).

**B2.** `static` on a class member function means "no `this`" — it still needs an
**out-of-line definition** somewhere (unless defined in the class body). The error
means no TU defined `Config::instance()`. (Do not confuse it with namespace-scope
`static`, which would give internal linkage.)

**B3.** `static` at namespace scope in a header gives **every including TU its own
copy** of the variable — five TUs, five caches. It compiles and links silently.
Fix: `inline std::vector<int> g_cache;` (C++17) for one shared object, or better,
an accessor function returning a reference to a function-local static, or best,
don't have a mutable global.

**B4.** The template definition is not visible in `main.cpp`, so the compiler cannot
instantiate `Vec<int>::grow()` and nobody else does either. Fixes: move the
definition into the header, or add `template class Vec<int>;` (explicit
instantiation) to `vec.cpp` and `extern template class Vec<int>;` to the header —
which limits you to the instantiations you listed but cuts compile time.

**B5.** Static initialization order fiasco: `g_logger` and `g_config` are in
different TUs, and the cross-TU order of dynamic initialization is **unspecified**,
so `g_logger`'s constructor sometimes runs first and reads an unconstructed
`Config`. "Sometimes" tracks link order, which is why it appears and disappears.
Fix: `Config& config() { static Config c{"/etc/app.conf"}; return c; }` and have the
logger call it, or make the configuration constant-initialized.

**B6.** ODR violation: `sizeof(Tracker)` differs between the two TUs, so a
`Tracker` constructed in one and used in the other has the wrong size and member
offsets → heap corruption or garbage reads, with a clean link and no warning. Fix:
never let a macro change a type's layout in a shared header; put debug fields in a
separate side table, and ensure `NDEBUG` is set uniformly for the whole build
(CMake `target_compile_definitions` on the *target*, not per-file).

**B7.** Two ABI breaks in one: a new **data member** changes `sizeof(Detector)` and
every member offset, and inserting `reset()` **before** `detect()` in the class
shifts every subsequent vtable slot — so an old binary calling `detect()` now jumps
to `reset()`. An application built against v1.0 and loaded with the v1.1 `.so`
misbehaves catastrophically. Fixes: bump the SO version (`libdetector.so.2`) so the
loader refuses the mismatch; or never change a published interface — freeze
`DetectorV1` and add a `DetectorV2`; or use PIMPL so layout changes stay behind an
opaque pointer; or expose only an `extern "C"` factory + a frozen pure-virtual
interface whose vtable you only ever **append** to.

**B8.** Does not compile: two `extern "C"` functions cannot share a name because C
linkage means no mangling, so both would want the symbol `process`. Fix: give them
distinct names (`process_str`, `process_str_n`) — which is exactly why C APIs are
full of `_n`/`_ex` suffixes — or keep the overloads in C++ and expose one
`extern "C"` entry point.

---

**C1.**
```
autonomy_core/
├── CMakeLists.txt
├── include/autonomy_core/          <- the ONLY public directory
│   ├── frame.hpp                   (trivially copyable wire types + static_asserts)
│   ├── detector.hpp                (the frozen abstract interface + extern "C" factory)
│   └── version.hpp                 (ABI version constants)
├── src/                            <- private headers + implementation
│   ├── detector_impl.cpp
│   └── internal/ring_buffer.hpp
├── tests/
└── cmake/autonomy_coreConfig.cmake.in
```
```cmake
add_library(autonomy_core src/detector_impl.cpp)
target_include_directories(autonomy_core
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE src)                                  # src/ is NOT visible to consumers
target_compile_features(autonomy_core PUBLIC cxx_std_20)
set_target_properties(autonomy_core PROPERTIES
    CXX_VISIBILITY_PRESET hidden VISIBLE_INLINES_HIDDEN ON
    VERSION 1.2.0 SOVERSION 1)                    # SOVERSION == the ABI contract
target_compile_definitions(autonomy_core PRIVATE AUTONOMY_CORE_BUILDING)
```
Key points to say:
- **Public vs. private include dirs** (`PUBLIC`/`PRIVATE`) is what stops 40 targets
  from depending on internal headers, and it is enforced by the build system rather
  than a convention.
- **One flag set for everyone**: put `NDEBUG`, feature toggles, and `-std` on the
  *target* as `PUBLIC`/`INTERFACE` compile definitions so no consumer can compile the
  header differently — this is the ODR defence.
- **ABI**: `SOVERSION` so the loader refuses a mismatch; `-fvisibility=hidden` +
  an explicit export macro so the dynamic symbol table is exactly the intended API;
  only trivially copyable, fixed-width, standard-layout types in the public headers,
  with `static_assert`s on `sizeof`/`offsetof` so a layout change fails the build;
  PIMPL for anything whose state might grow; frozen pure-virtual interfaces that you
  only append to; `extern "C"` create/destroy pairs so allocation stays on one side.
- **Build speed**: minimal public headers, forward declarations, no
  `#include <vector>` in a public header if a `std::span` will do, `extern template`
  for the two or three heavy instantiations, `ccache`, `ninja`, and a CI job that
  fails if the public-header preprocessed size grows past a budget.
- An **ABI dump check in CI** (`abi-compliance-checker` / `abidiff`) that fails the
  build on an unintended ABI change. That answer is what distinguishes someone who
  has shipped a library.

**C2.** Measure first (`-ftime-trace`, `-ftime-report`, `ninja -t graph`), then:
1. **`ccache`/`sccache` + `ninja`** — near-zero effort, often 50-80% off incremental
   builds.
2. **A faster linker**: `mold` or `lld` instead of BFD ld — 6 minutes of link time
   typically drops to under 1. Biggest single win here, because link is 13% of the
   wall clock and `mold` is ~10x.
3. **Cut the include graph**: IWYU, forward declarations, move implementation-only
   includes into `.cpp`s, PIMPL the two or three worst headers. This is the highest
   *ceiling* but the most work.
4. **Kill template instantiation cost**: `extern template` for the heavy ones,
   concepts instead of `enable_if` chains, `constexpr` functions instead of recursive
   class templates, and hoist type-independent code out of templates.
5. **Parallelism and machine**: `-j$(nproc)`, `distcc`/`icecc` or a remote-execution
   backend, more RAM so `/tmp` and the object cache stay in page cache, and
   `-g1`/split DWARF (`-gsplit-dwarf`) since debug info is often half the object
   size and most of the link time.
6. **Structural**: precompiled header for the stable third-party surface; unity
   builds for cold directories; split the monolithic library into targets so a change
   rebuilds less; C++20 modules for new code.
Then: **track build time as a CI metric with a budget**, or it regresses back.

**C3.** Diagnosis walk-through:
1. Confirm the observation — same objects, two link orders, two behaviours. If so,
   the program depends on something the linker chose arbitrarily, which means an
   **ODR violation** or a **duplicate symbol resolved differently**.
2. Look for duplicate definitions:
   `nm -C --defined-only *.o *.a | sort -k3 | uniq -d -f2`, and
   `nm -DC libfoo.so | grep ' [TWV] '` on each library — two libraries defining the
   same `class Logger`, or two versions of a vendored dependency, is the classic
   cause. `W` (weak) symbols are where `inline`/template definitions get merged, and
   the linker keeps the **first** one it sees — hence the link-order dependence.
3. Rebuild with `-flto` and read `-Wodr`; gcc reports the exact type and the first
   differing field.
4. Diff the effective flags per TU (`compile_commands.json`) looking for a macro
   that changes a header — `NDEBUG`, `_GLIBCXX_DEBUG`, `_GLIBCXX_USE_CXX11_ABI`,
   `#pragma pack`, a feature toggle. `_GLIBCXX_DEBUG` in some TUs and not others is
   a notorious instance because it changes `std::vector`'s layout.
5. Check **static initialization order** as the other link-order-sensitive
   mechanism: link order determines the order of dynamic initializers, so a global
   that reads another global changes behaviour. `__attribute__((constructor))`
   priorities and `nm` on `_GLOBAL__sub_I_*` symbols help here.
6. Fix the root cause (unify flags, remove the duplicate symbol, replace the global
   with construct-on-first-use), then add the guard: `-Wodr` under LTO in CI, one
   target-level definition of every flag, and `-fvisibility=hidden` so plugin
   symbols cannot collide in the first place.
