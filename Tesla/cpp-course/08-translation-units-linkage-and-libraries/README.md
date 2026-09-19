# 08 — Translation Units, Linkage, the ODR, `#include`, Modules, Libraries, ABI

Course chapters: **13 (Translation Units I), 14 (Translation Units II)**

This is the "why doesn't it link?" section. A foundations engineer is the person
everyone asks when the link fails, so know it cold.

---

## 1. The build pipeline

```
foo.cpp ──preprocess──> foo.ii ──compile──> foo.s ──assemble──> foo.o ──┐
bar.cpp ──────────────────────────────────────────────────────> bar.o ──┼─link─> a.out
                                                     libm.so, libstdc++ ┘
```

A **translation unit (TU)** is one source file after preprocessing — i.e. the `.cpp`
plus every header it transitively includes. The compiler sees one TU at a time and
knows nothing about the others; the **linker** resolves symbols between them and
knows nothing about types.

```bash
g++ -E foo.cpp            # stop after preprocessing (see the real TU)
g++ -c foo.cpp -o foo.o   # compile only
g++ foo.o bar.o -o app    # link
nm -C foo.o               # symbols, demangled
nm -C --defined-only foo.o
nm -C -u foo.o            # UNDEFINED symbols — the ones the linker must find
c++filt _ZN5Vec3plERKS_   # demangle by hand
readelf -d app            # dynamic dependencies
ldd app                   # shared libraries actually resolved
objdump -dC foo.o         # disassemble
```

## 2. Scope, linkage, storage duration — three orthogonal things

**Scope** (where the name is visible): block, function, class, namespace, global.

**Linkage** (whether the name can refer to the same entity from elsewhere):
- **no linkage** — local variables, function parameters
- **internal linkage** — visible only in this TU: `static` at namespace scope,
  anything in an anonymous namespace, `const`/`constexpr` namespace-scope variables
  (unless `extern`), type aliases, enums
- **external linkage** — visible to other TUs: non-`static` functions and
  variables, `extern`, class member functions, `inline` functions, templates
- **module linkage** (C++20) — visible only within the module

**Storage duration** (how long the object lives): automatic, static, thread
(`thread_local`), dynamic.

```cpp
static int s_counter;                // internal linkage, static duration
namespace { int a_counter; }         // same, and works for types too — prefer this
extern int g_counter;                // declaration of an external-linkage variable
int g_counter = 0;                   // its one definition
const int kMax = 10;                 // internal linkage by default in C++ (not C!)
extern const int kShared;            // external linkage const
inline int helper() { return 1; }    // external linkage, definable in every TU
inline int g_inline_var = 0;         // C++17: one variable, header-only
thread_local int t_id;               // one per thread
constinit int c_init = compute();    // constant-initialized, no ordering fiasco
```

The `static` keyword has **four** unrelated meanings — a favourite question:
1. namespace scope → internal linkage
2. block scope → static storage duration (initialized once, thread-safe since
   C++11)
3. class data member → one per class, not per object
4. class member function → no `this`

## 3. The One Definition Rule

1. Every TU may contain **at most one** definition of any variable, function,
   class, enum, or template.
2. The **program** must contain exactly one definition of every non-`inline`
   function or variable that is *used*.
3. Some entities may be defined in **more than one TU** — classes, `inline`
   functions/variables, templates, enums, `constexpr` — **provided every definition
   is token-for-token identical and means the same thing in each TU**.

Violating rule 3 is **IFNDR** (ill-formed, no diagnostic required): the linker
picks one definition arbitrarily and you get a silent, impossible-to-debug bug.
This is the single most important sentence in this section.

How rule-3 violations actually happen:
```cpp
// a.cpp
#define DEBUG 1
#include "widget.hpp"   // struct Widget { int a; #ifdef DEBUG int dbg; #endif };
// b.cpp
#include "widget.hpp"   // different sizeof(Widget)!  -> heap corruption
```
Also: different `-D` flags, different `-std=`, different `NDEBUG`,
different struct packing pragmas, an anonymous-namespace type leaking into an
`inline` function's signature, or two different libraries both defining
`class Logger`.

Mitigations to name: build **every** TU with the same flags (one CMake target-level
`target_compile_definitions`), keep configuration out of header-visible layout, use
an ABI tag / inline namespace when layout must change, and run the
**`-Wodr`** check with LTO (gcc detects many ODR violations at link time).

```cpp
// The `inline` rule in one line: `inline` does NOT mean "inline this call".
// It means "this definition may appear in every TU; the linker will merge them."
inline int square(int x) { return x * x; }   // header-safe
inline constexpr int kMax = 10;              // C++17 header-safe variable
```
`constexpr` functions are implicitly `inline`. `constexpr` variables at namespace
scope are implicitly `const` and therefore internal-linkage unless you add
`inline`.

### Static initialization order fiasco
Namespace-scope objects with **dynamic** initialization are initialized in
unspecified order **across** TUs (in declaration order within a TU). So:
```cpp
// a.cpp
extern Config g_config;
Logger g_logger{g_config.level};      // may run BEFORE g_config is constructed
```
Fixes, in order of preference:
1. Don't have mutable global state.
2. `constinit` / `constexpr` so initialization is **constant** (happens at compile
   time, no ordering).
3. **Function-local static** (the "construct on first use" idiom) — initialized on
   first call, and thread-safe since C++11:
   ```cpp
   Config& config() { static Config c{load()}; return c; }
   ```
4. Nifty-counter / `std::call_once` if you also need deterministic teardown.

Destruction order is the reverse of construction, which creates the mirror problem
(the "static **de**initialization" fiasco) — a destructor touching an
already-destroyed global. Function-local statics leak deliberately in some codebases
(`static T* p = new T;`) to avoid it.

## 4. `#include` hygiene

```cpp
#pragma once                       // or an include guard
#include <vector>                  // 1. C++ standard library
#include <third_party/foo.hpp>     // 2. third party
#include "project/bar.hpp"         // 3. your project
#include "local_helper.hpp"        // 4. this directory
// Order matters: put your OWN header first in the .cpp (self-sufficiency check),
// then go from most general to most specific so a missing include is caught.
```

- `#include <x>`: implementation-defined search, normally the system paths (`-I`,
  `-isystem`).
- `#include "x"`: first relative to the including file, then the `<>` path.
- **Forward declare instead of including** whenever you only need a pointer,
  reference, or a function declaration. This is the main build-time lever.
- **Circular includes** are always a design error: break them with a forward
  declaration, or extract the shared type into a third header.
- Every header must be **self-sufficient** (compiles standalone) and **idempotent**.

Common linker errors and their real causes:

| Error | Cause |
| --- | --- |
| `undefined reference to 'f()'` | declared but never defined; or defined in a `.cpp` you did not link; or defined `static` in another TU; or C/C++ linkage mismatch (`extern "C"`) |
| `undefined reference to vtable for X` | a virtual function (often the destructor) is declared but not defined |
| `multiple definition of 'g'` | a non-`inline` function or variable defined in a header |
| `undefined reference to 'Foo<int>::bar()'` | template defined in a `.cpp`, used from another TU (needs the definition in the header, or an explicit instantiation) |
| link succeeds, crashes at runtime | ODR violation, or an ABI mismatch between libraries |

## 5. C++20 modules (know the concept, expect to still use headers)

```cpp
// math.cppm  — a module interface unit
export module math;                 // module declaration
import std;                          // (C++23) or #include in the global fragment

export int square(int x) { return x * x; }   // exported
int helper(int x) { return x; }              // internal to the module
export namespace geo { struct Point { int x, y; }; }

// consumer.cpp
import math;
int main() { return square(4); }
```
Terminology: *module unit*, *module interface unit* (`export module M;`),
*module implementation unit* (`module M;`), *partition* (`export module M:part;`),
*global module fragment* (`module;` then `#include`s, for legacy headers),
*private module fragment* (`module :private;`).

Why they matter: a module is compiled once into a BMI and *imported*, so you do not
re-parse the same 50k lines in 500 TUs; macros do not leak across an `import`;
`export` makes the interface explicit; and the ODR hazards above largely vanish.
Why you will still use headers: toolchain and build-system support is still
uneven, and every dependency has to cooperate.

## 6. Static vs. dynamic libraries, and ABI

```bash
# static library: an archive of .o files
g++ -c a.cpp b.cpp
ar rcs libfoo.a a.o b.o
g++ main.cpp -L. -lfoo -o app          # symbols copied INTO the executable

# shared library: position-independent code, resolved at load time
g++ -fPIC -c a.cpp b.cpp
g++ -shared -o libfoo.so a.o b.o
g++ main.cpp -L. -lfoo -Wl,-rpath,'$ORIGIN' -o app
```

| | static (`.a`) | shared (`.so`) |
| --- | --- | --- |
| link time | slower link, faster startup | fast link, dynamic symbol resolution at load |
| binary size | bigger executable; duplicated in every consumer | one copy on disk, shared page cache across processes |
| updating | relink everything | drop in a new `.so` (if the ABI holds) |
| performance | direct calls, LTO across the whole program, no PLT | PLT/GOT indirection on cross-library calls, no cross-library inlining |
| deployment | one file | must be found at runtime (`rpath`, `LD_LIBRARY_PATH`) |

For an embedded/automotive target, static linking plus LTO is usually preferred:
deterministic, no loader surprises, better optimization. Say that.

**Link order matters for static libraries**: the linker processes left to right and
takes only the objects it needs at that moment, so `-lfoo` must come *after* the
objects that use it. `--start-group/--end-group` handles cycles.

### ABI — application binary interface
The ABI is everything two separately compiled binaries must agree on: name
mangling, struct layout and padding, calling convention, vtable layout, exception
unwinding tables, the standard library's own types.

**What breaks ABI (never do it in a published interface):**
adding/removing/reordering virtual functions; adding or reordering data members;
changing a type's size or alignment; changing a function signature or return type;
changing an inline function's body that other binaries have already inlined;
changing default arguments; changing an enum's underlying type; switching
`-D_GLIBCXX_USE_CXX11_ABI`.

**How to keep it stable:** an `extern "C"` factory + pure-virtual interface with a
frozen vtable, or PIMPL (all state behind an opaque pointer), only trivially
copyable fixed-width types across the boundary, `-fvisibility=hidden` plus explicit
export macros, explicit versioning (`abi_version()`, inline namespaces, versioned
symbols in a linker script), and `noexcept` on every boundary function.

```cpp
extern "C" {                        // C linkage: no mangling, no overloading
    int tesla_init(const Config* cfg);
}
// name mangling exists because C++ has overloading, namespaces, and templates:
//   _ZN5tesla8autonomy4tickEi  ->  tesla::autonomy::tick(int)
```

---

## Traps checklist

1. A TU is the `.cpp` **plus all its headers**; the compiler sees one at a time.
2. `static` has four unrelated meanings.
3. Namespace-scope `const` has **internal** linkage in C++ (unlike C).
4. `inline` means "definable in every TU", **not** "please inline this".
5. An ODR violation is IFNDR: the linker silently picks one definition.
6. Different macros/flags per TU over a shared header = ODR violation = heap
   corruption.
7. Cross-TU dynamic initialization order is unspecified → function-local static or
   `constinit`.
8. Function-local statics are thread-safe since C++11 (and cost one guard check).
9. Template definitions must be visible where instantiated, or explicitly
   instantiated.
10. `undefined reference to vtable` = an undefined virtual function, usually the
    destructor.
11. Static-library link order is left to right.
12. Any change to a class's layout or vtable is an ABI break.
