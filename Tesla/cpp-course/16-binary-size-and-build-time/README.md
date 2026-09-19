# 16 — Binary Size and Build Time

Course chapters: **28 (Binary Size), 29 (Build Time)**

Both are *platform team* problems. On an embedded automotive target, flash is finite and
a 40-minute build costs every engineer on the project every day. Owning these metrics is
exactly what a foundations engineer does.

---

## 1. Why binary size matters (and what it is made of)

Flash/OTA budget, load time, and — the one people forget — **instruction-cache
pressure**: a smaller hot path is a faster hot path, which is why `-Os` sometimes beats
`-O2` on a large branchy binary.

```bash
size -A ./app                 # per-section: .text .rodata .data .bss .eh_frame
nm -C --size-sort -S ./app | tail -30        # the 30 biggest symbols
bloaty ./app -d compileunits,symbols         # THE tool: attributes size to sources
bloaty -d symbols old.elf -- new.elf         # diff two builds
objdump -h ./app              # section headers
readelf -S ./app
strip --strip-all ./app       # remove symbols (keep a separate unstripped copy!)
objcopy --only-keep-debug app app.debug && objcopy --add-gnu-debuglink=app.debug app
```
Typical composition of a C++ binary: `.text` (code), `.rodata` (constants, string
literals, vtables, type_info names), `.eh_frame`/`.gcc_except_table` (**exception unwind
tables — often 5-15%**), `.data`/`.bss`, debug info (`.debug_*`, often larger than
everything else combined — that is why you strip and keep a separate symbol file).

## 2. Flags that change size

```bash
-Os            # optimize for size (≈ -O2 minus the size-increasing passes)
-Oz            # clang: even smaller, will trade speed
-O2 vs -O3     # -O3 inlines and unrolls more: bigger
-flto          # usually SMALLER (cross-TU dead code elimination) and faster
-ffunction-sections -fdata-sections -Wl,--gc-sections   # drop unreferenced sections
-Wl,--icf=all  # lld/gold: identical code folding (merge identical functions)
-fvisibility=hidden -fvisibility-inlines-hidden          # smaller dynamic symbol table
-fno-exceptions        # removes unwind tables: often 5-15%
-fno-rtti              # removes type_info: a few %
-fno-asynchronous-unwind-tables -fno-unwind-tables
-g0 / -g1              # debug info; or -gsplit-dwarf to move it out of the binary
-fno-ident -Wl,--build-id=none -Wl,-s
--specs=nano.specs     # newlib-nano on embedded
```
`--gc-sections` plus `-ffunction-sections` is the highest-value pair for an embedded
build: without them, linking one function from a translation unit pulls in the whole
object file.

## 3. Coding decisions that change size

- **Templates** are the biggest lever. Every distinct instantiation is separate code.
  - Hoist type-independent code out of the template into a non-template base or free
    function (the **thin template** idiom).
  - `extern template` to instantiate once instead of per TU.
  - Erase where the type does not matter: a `std::span<std::byte>`-based core with a thin
    typed wrapper.
  - Watch out for `std::function`, `std::regex`, iostreams, and `std::to_string` —
    each pulls in a lot.
- **Inlining**: `[[gnu::noinline]]` on cold, large functions; `-finline-limit`;
  do not force-inline something called from 200 places.
- **Polymorphic classes** cost a vtable plus `type_info` plus the names as strings.
- **Exceptions**: each `try`/destructor scope adds landing pads and unwind table entries.
- **Static storage duration** objects with dynamic initialization add a constructor call
  and a guard per object; `constinit`/`constexpr` makes them pure data in `.rodata`.
- **String literals** are deduplicated per TU but not always across TUs; `__FILE__` in a
  logging macro embeds the full path in every call site (use
  `-ffile-prefix-map=/build=.`, or a short-name trick).
- **`std::iostream`** pulls in locale machinery — often 100-300 KB. `std::format`/
  `printf` are much smaller. On embedded, avoid iostreams entirely.

## 4. Why build time matters, and where it goes

A build that takes 40 minutes changes how people work: fewer iterations, bigger
speculative changes, more context switching. It is a *productivity* metric, and it should
be tracked like a performance metric.

The costs, in the order they usually dominate:
1. **Header parsing** — every TU re-parses every header it includes. `#include <vector>`
   is ~20k lines; a project header that transitively pulls in Eigen is ~100k lines, paid
   500 times.
2. **Template instantiation** — the compiler does real work per instantiation, and
   SFINAE/`enable_if` chains multiply it.
3. **Optimization** — `-O2`/`-O3` and especially `-flto`.
4. **Linking** — often 30-50% of the wall clock on a large project, and it is
   single-threaded with the default BFD `ld`.
5. **Debug info generation** — surprisingly large.

Measure, do not guess:
```bash
clang++ -ftime-trace file.cpp      # per-TU JSON -> chrome://tracing, shows which
                                   # header and which template cost what
g++ -ftime-report                  # per-pass timing
ninja -t graph | dot -Tpng         # dependency graph
ninja -d stats ; ninjatracing .ninja_log > trace.json
include-what-you-use               # unnecessary includes
clang-include-graph / cpp-dependencies
Templight                          # template instantiation profiler
ClangBuildAnalyzer                 # aggregates -ftime-trace across the project
```

## 5. Fixes, in order of payoff

1. **`ccache`/`sccache` + `ninja`**. Near-zero effort, huge effect on incremental builds.
2. **A faster linker**: `mold` (or `lld`). Link time typically drops 5-10x. This is the
   single biggest one-line win on a large project.
3. **Cut the include graph**: forward declare, move implementation-only includes into the
   `.cpp`, **PIMPL** the worst offenders, split monolithic headers. Highest ceiling,
   most work. Use IWYU to find them.
4. **`-gsplit-dwarf`** (debug info into `.dwo` files, not the objects) and `-g1` where
   full debug info is not needed — cuts both compile and link time.
5. **Reduce template work**: concepts instead of `enable_if` chains (rejected before
   instantiation), `constexpr` functions instead of recursive class templates,
   `extern template` for the heavy instantiations, and the thin-template idiom.
6. **Precompiled headers** for the stable third-party surface
   (`target_precompile_headers`), and **unity builds** (`CMAKE_UNITY_BUILD`) for cold
   directories — both are blunt instruments that trade incremental-build granularity for
   full-build speed.
7. **Distributed compilation** (`distcc`, `icecc`, or a remote-execution backend) and
   simply `-j$(nproc)` with enough RAM.
8. **Split targets** so a one-line change does not rebuild the world, and keep the
   dependency graph shallow.
9. **C++20 modules** for new code: a module interface is compiled once into a BMI and
   imported, rather than re-parsed per TU. This is the real long-term fix; tooling
   support is the constraint.

## 6. Things that cost more than people think

| Thing | Cost |
| --- | --- |
| `#include <regex>` | very large code size and build time; usually avoidable |
| `#include <iostream>` | pulls in locales; static init in every TU |
| a `std::function` member in a hot class | 32 bytes, an allocation, a virtual call |
| `-O3` over `-O2` | bigger `.text`, sometimes slower |
| deep `enable_if` SFINAE | the compiler re-substitutes per candidate per call |
| a template in a header used with 30 types | 30 copies of the code |
| `__FILE__` in a log macro | the absolute path, in `.rodata`, per call site |
| full debug info (`-g3`) | frequently more than half the object size |
| the default BFD linker | single-threaded; often half your incremental build |

---

## The two workflows to be able to recite

**Binary too big:**
1. `size -A` to see which section. If `.debug_*` — strip and split. If `.eh_frame` —
   consider `-fno-exceptions`. If `.text` — continue.
2. `bloaty -d compileunits,symbols` to attribute it to source files and symbols.
3. `nm --size-sort -S | tail` for the individual offenders — usually a template
   instantiated many times, or an inlined function duplicated everywhere.
4. Flags first (`-Os`, `--gc-sections`, `--icf=all`, `-fvisibility=hidden`, LTO), then
   code (de-templatize, thin template, `extern template`, remove iostreams/regex).
5. Re-measure, and **add a size budget to CI** with a `bloaty` diff on every PR.

**Build too slow:**
1. Measure: `-ftime-trace` + ClangBuildAnalyzer, and split compile time from link time.
2. `ccache` + `ninja` + `mold`. (Minutes of work.)
3. Include graph: IWYU, forward declarations, PIMPL.
4. Template cost: concepts, `extern template`, thin templates.
5. `-gsplit-dwarf`, PCH, unity builds for cold directories.
6. **Track build time in CI as a metric with a budget**, or it regresses back.

---

## Traps checklist

1. Debug info is usually the biggest thing in an unstripped binary — strip, and keep the
   symbols separately (`--add-gnu-debuglink`) or you cannot symbolize a core dump.
2. `-ffunction-sections -fdata-sections` is useless without `-Wl,--gc-sections`.
3. LTO usually makes binaries **smaller**, not bigger.
4. `-fno-exceptions` is an ABI decision: every library in the process must agree.
5. `-O3` is bigger and not always faster.
6. Every template instantiation is separate code.
7. `#include <iostream>` adds static initializers to every TU that includes it.
8. `__FILE__` embeds an absolute build path (also a reproducibility problem —
   `-ffile-prefix-map`).
9. The default linker is single-threaded; `mold`/`lld` is often the biggest build win.
10. PCH and unity builds speed up full builds and can *slow down* incremental ones.
11. Measure build time before optimizing it; the bottleneck is usually not where you
    think.
