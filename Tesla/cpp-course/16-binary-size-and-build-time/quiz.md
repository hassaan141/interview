# 16 — Mock interview questions

## A. Rapid fire

1. Name the main sections of an ELF binary and what lives in each.
2. What is usually the largest component of an unstripped C++ binary?
3. How do you strip a binary and still be able to symbolize a core dump?
4. Roughly what fraction of a binary is exception unwind tables?
5. What does `-ffunction-sections -fdata-sections` do, and what is it useless without?
6. Does LTO usually make a binary bigger or smaller? Why?
7. Why is `-O3` usually bigger than `-O2`, and when is it also *slower*?
8. Why can `-Os` be faster than `-O2` on a large binary?
9. What is identical code folding?
10. Why does `-fvisibility=hidden` reduce size, and what else does it improve?
11. Is `-fno-exceptions` a per-file decision? Why not?
12. Why does every template instantiation cost code size?
13. What is `extern template` for?
14. What is the "thin template" idiom?
15. Name three standard headers that are expensive in size or build time.
16. What does `__FILE__` in a logging macro cost, and what fixes it?
17. Where does build time actually go? Rank the four main contributors.
18. What is the single biggest one-line build-time win on a large project?
19. What does `-gsplit-dwarf` do?
20. What is `ccache` and when does it not help?
21. Trade-off of precompiled headers? Of unity builds?
22. How do modules fix the underlying problem?
23. How do you measure where compile time goes? Where link time goes?
24. How do you stop both metrics from regressing?

## B. Diagnose

**B1.** A firmware image grew from 1.8 MB to 2.4 MB after one merge. Flash limit is
2.0 MB.

**B2.** An incremental build after touching one `.cpp` takes 90 seconds, of which 70 is
after the last compile finishes.

**B3.** Touching one header rebuilds 380 of 500 translation units.

**B4.** Your `.text` is 40% one template, instantiated with 30 different types.

**B5.** A clean build is 12 minutes on CI and 40 minutes on a developer laptop.

**B6.** After enabling `-flto`, binary size dropped 8% but link time went from 20 s to
6 minutes.

## C. Whiteboard

**C1.** You own the build for a 500-TU project: 40 min clean, 90 s incremental, 6 min
link. Give an ordered plan with expected wins.

**C2.** The embedded target has 2 MB of flash and you are 400 KB over. Walk through
getting under budget, from cheapest to most invasive.

**C3.** Design the CI gates for binary size and build time so neither can regress
silently.

---
---

# Answers

**A1.** `.text` (executable code), `.rodata` (constants, string literals, vtables,
`type_info` name strings), `.data` (initialized writable globals), `.bss` (zero-init
globals — occupies no file space), `.eh_frame` + `.eh_frame_hdr` +
`.gcc_except_table` (exception unwind information), `.init_array` (static constructors),
`.dynsym`/`.dynstr`/`.rela.*` (dynamic linking), and `.debug_*` (debug info). Inspect
with `size -A`, `readelf -S`, `objdump -h`.

**A2.** **Debug info** (`.debug_*`). In the measurement in `examples.cpp`, `-O2 -g` is
2,212,256 bytes and the same binary stripped is 134,472 — about **16x**. That is why the
shipped artifact is always stripped and the symbols are archived separately.

**A3.**
```bash
objcopy --only-keep-debug app app.debug
objcopy --add-gnu-debuglink=app.debug app
strip --strip-all app        # or: strip --strip-debug to keep the symbol table
```
Then archive `app.debug` alongside the build, keyed by the **build-id**
(`readelf -n app | grep 'Build ID'`), so a core dump collected in the field can be
symbolized months later. `-gsplit-dwarf` achieves something similar at compile time.
Losing the symbols is how you end up with an unactionable crash report.

**A4.** Typically **5-15%** of the binary. In the measured example, `.eh_frame` +
`.eh_frame_hdr` + `.gcc_except_table` is 10,972 + 1,484 + 2,338 ≈ 14.8 KB of a 172 KB
binary, about 8.6%. It grows with the number of scopes containing destructible objects.

**A5.** It puts each function and each data object in its own ELF section, so the linker
can discard them individually. It is **useless without `-Wl,--gc-sections`**, which is
what actually removes the unreferenced sections. Without both, linking one function from
an object file pulls in the whole object file — which is why this pair is the
highest-value size flag on an embedded build.

**A6.** Usually **smaller**. LTO sees the whole program, so it can do cross-TU dead-code
elimination, devirtualize and then remove unreachable virtual functions, internalize
symbols (nothing outside can call them, so they need no separate copy), and merge
duplicate code. It can grow `.text` where it decides to inline more aggressively across
TUs, but net is typically −5 to −15%. The measurement in `examples.cpp`: 146,952 →
124,792 bytes, about −15%.

**A7.** `-O3` adds aggressive inlining, loop unrolling, and vectorization, all of which
duplicate code. It is **also slower** when the binary becomes instruction-cache bound: a
hot loop that no longer fits in L1i, or a function inlined into 50 call sites evicting
other hot code. Branchy, call-heavy workloads (interpreters, parsers, state machines) are
where `-O2` or `-Os` frequently wins. Measure per binary.

**A8.** Because a smaller hot path fits in the **instruction cache** and the ITLB, and
i-cache misses are as expensive as data misses. `-Os` is roughly `-O2` minus the passes
that grow code, so on a large branchy binary you lose a little per-loop throughput and
gain more from fetch locality.

**A9.** A linker pass (`-Wl,--icf=all` with lld or gold) that finds functions with
**identical machine code** and folds them into one. C++ benefits enormously because
template instantiations over different types frequently compile to byte-identical code
(`std::vector<int>` vs `std::vector<long>` on LP64). `--icf=safe` only folds functions
whose address cannot be observed, since folding changes function-pointer identity.

**A10.** With `-fvisibility=hidden` symbols are not exported from a shared object unless
explicitly marked, so the **dynamic symbol table** (`.dynsym`/`.dynstr`) and relocations
shrink — often significantly for a C++ library with many template instantiations. It also
**improves performance and enables optimization**: the compiler and LTO know nothing
outside can override or interpose these symbols, so they can inline and devirtualize;
load time improves because there are fewer symbols to resolve; and you get no accidental
ABI surface and no symbol collisions between plugins.

**A11.** **No** — it is effectively an ABI-wide decision. An exception thrown in a TU
compiled with exceptions cannot propagate through a frame compiled `-fno-exceptions`
(there is no unwind information for it), so it terminates. Mixing also means the standard
library you link must be consistent (`std::vector` cannot report `bad_alloc`). So the
whole process — including every library — must agree, which is why it is a project-level
decision made at the start.

**A12.** Because each instantiation is a **distinct function** with its own machine code
— the compiler substitutes the type and compiles the body again. `std::vector<int>` and
`std::vector<Frame>` share no code. The linker deduplicates *identical* instantiations
across TUs (weak/COMDAT symbols) and ICF can fold byte-identical ones, but genuinely
different types mean genuinely different code.

**A13.** `extern template class Vec<int>;` in a header suppresses implicit instantiation
in every TU that includes it, and `template class Vec<int>;` in exactly one `.cpp`
instantiates it once. You pay the compile cost once instead of per TU, and the linker has
one copy to keep instead of N to merge. It cuts both build time and (before dedup) object
size — at the cost of having to enumerate the instantiations you support.

**A14.** Move the code that does **not** depend on the template parameter out of the
template and into a non-template base class or free function, leaving a thin typed wrapper
that just casts. Classic example: a `Vector<T>` whose storage management is implemented
once in a `VectorBase` over `void*` plus an element size, with `Vector<T>` providing the
typed interface. One copy of the logic instead of N. The cost is lost type information
inside the shared core, so it has to be written carefully.

**A15.** `<regex>` (enormous code size *and* build time — usually replaceable),
`<iostream>` (locale machinery, often 100-300 KB, plus a static initializer in every TU
that includes it), `<chrono>` and `<ranges>` and `<format>` (heavy on **compile time**
specifically), and any header that transitively pulls in Eigen or a similar
template-heavy library. In the measurement above, dropping `<regex>` + `<iostream>` +
`<map>` took the binary from 172 KB to 16 KB (**10.8x**) and the build from 4.4 s to
0.16 s (**27.7x**).

**A16.** It embeds the **absolute source path** as a string literal in `.rodata` at every
call site — so a project with 5000 log statements under a long build path can waste
hundreds of kilobytes, and it also breaks **reproducible builds** (the binary differs
between machines). Fixes: `-ffile-prefix-map=/long/build/path=.` (or
`-fmacro-prefix-map`), C++20 `std::source_location` (which the implementation can encode
more compactly), or a build step that replaces paths with interned short IDs.

**A17.** (1) **Header parsing** — every TU re-parses every transitively included header;
this usually dominates and it is why `#include <vector>` in a header costs 500 times.
(2) **Template instantiation** — real work per instantiation, multiplied by SFINAE
overload sets. (3) **Optimization** — `-O2`/`-O3`, and especially LTO. (4) **Linking** —
often 30-50% of wall clock on a large project, and single-threaded with the default BFD
`ld`. (Plus debug-info generation, which shows up in both compile and link.)

**A18.** Switching the **linker** to `mold` (or `lld`). Link is often 30-50% of the wall
clock, it is single-threaded with BFD `ld`, and `mold` is typically 5-10x faster — a
one-line change (`-fuse-ld=mold`) with no source impact. `ccache` is a close second for
incremental/CI builds.

**A19.** It writes debug info into separate `.dwo` files instead of into the object files,
so the objects are much smaller and — importantly — the **linker does not have to read
and relocate all that debug info**, which is often the biggest single component of link
time. The debugger finds the `.dwo` files via a reference. Combine with a `dwp` packaging
step for distribution.

**A20.** A compiler cache: it hashes the preprocessed source plus the compiler and flags,
and returns a cached object file on a hit. It does not help when: the hash changes every
time (`__DATE__`/`__TIME__`, an absolute path in the command line, a generated header
with a timestamp), the cache is cold (a first CI build on a fresh runner — fix with a
shared/remote cache), the build is dominated by **link** time (ccache does not cache
links), or LTO moves the work to link time. It also cannot help a from-scratch build of
genuinely new code.

**A21.** **PCH**: one pre-parsed blob of stable headers is loaded per TU instead of
re-parsing them — big win on full builds; but it makes the dependency graph coarser (a
change to anything in the PCH rebuilds everything), the PCH itself is slow to build, and
it can hide missing includes so the code no longer compiles without it. **Unity builds**:
concatenating N `.cpp` files into one TU removes N−1 header re-parses and inter-TU
duplication — often 2-4x on a full build; but a one-line change now rebuilds the whole
chunk (worse *incremental* builds), it breaks `static`/anonymous-namespace assumptions and
can cause name collisions, and it hides missing includes. Both are blunt: prefer fixing
the include graph, and use these where you cannot.

**A22.** A module interface unit is compiled **once** into a binary module interface
(BMI) and then **imported** — the importer reads a pre-parsed, pre-semantically-analyzed
representation rather than re-parsing tens of thousands of lines of text. That attacks
contributor #1 directly. Modules also stop macros leaking across the boundary, make the
interface explicit via `export`, and remove include-order dependence and most ODR
hazards. The constraint is toolchain and build-system support, and that every dependency
has to cooperate.

**A23.** **Compile time**: `clang++ -ftime-trace` per TU (JSON for `chrome://tracing`,
attributing time to specific headers and template instantiations), aggregated across the
project with **ClangBuildAnalyzer**; gcc has `-ftime-report`. Also `Templight` for
template instantiation profiling and `include-what-you-use` /
`clang-include-graph` for the include graph. **Link time**: time the link step directly
(`ninja -d stats`, `ninjatracing .ninja_log`), compare linkers, and use `-Wl,--stats` /
`mold --stats`. The first thing to do is simply **split total time into compile vs. link**,
because the fixes are completely different.

**A24.** Make both **tracked metrics with a budget, enforced in CI**: a job that reports
`size -A`/`bloaty` totals and the build wall time for every commit, stores them as a time
series, fails the PR if binary size grows beyond a threshold, and posts a
`bloaty -d symbols old -- new` diff as a comment so the growth is attributed to a
specific symbol. Same for build time, with enough repetitions to beat the noise. Without
a gate both regress monotonically, because every individual commit's contribution looks
negligible.

---

**B1.** 600 KB from one merge is a specific, attributable change. (1) `bloaty -d
compileunits,symbols new.elf -- old.elf` — this tells you the answer in one command,
which is why you set up the tooling before you need it. (2) Expect one of: a newly
included heavy header (`<regex>`, `<iostream>`, a vendor SDK), a template newly
instantiated over many types, an unstripped or `-g` build slipping into the release
config, `--gc-sections` accidentally dropped, or a statically linked library that used to
be shared. (3) Check the section breakdown first (`size -A`): if it is `.debug_*` it is a
build-config regression, not a code regression, and the fix is 30 seconds. (4) Fix the
cause; if the code is genuinely needed, apply the cheap flags (`-Os` for the cold
modules, `--gc-sections`, `--icf=all`, LTO) to buy the headroom. (5) Add the CI size gate
so the next 600 KB is caught in the PR.

**B2.** 70 of the 90 seconds is **link time**. Compilation is already fine. Fixes, in
order: switch to **`mold`** or `lld` (`-fuse-ld=mold`) — expect 70 s → 5-15 s;
add **`-gsplit-dwarf`** so the linker does not process debug info; check whether **LTO**
is on for a debug/incremental build (it should not be); reduce the number of input
objects/archives and avoid `--start-group`; make sure you are not relinking more targets
than necessary. Do **not** start by optimizing headers — you measured, and headers are
not the problem.

**B3.** The header is at the top of the include graph and the graph is too deep. Fixes:
(1) `include-what-you-use` and `clang-include-graph` to see who really needs it;
(2) split the header — most consumers probably need one type out of it;
(3) **forward declare** instead of including in the 380 headers that only need a pointer
or reference; (4) **PIMPL** the class so its private members (and their includes) leave
the header, which means changing the implementation stops rebuilding consumers at all;
(5) make sure the header is not being modified for reasons that belong in the `.cpp`
(private helpers, member ordering). Long term, a **module** makes the interface change
the only thing that triggers a rebuild.

**B4.** One template, 30 instantiations, 40% of `.text`. In order: (1) **thin template** —
move everything that does not depend on `T` into a non-template base or a free function
taking `std::span<std::byte>` plus an element size, leaving a small typed shim; (2) check
whether 30 types are really needed, or whether several could share one instantiation
(e.g. all pointer types via `void*`, all 4-byte integers via `std::int32_t`);
(3) `extern template` for the ones used across many TUs; (4) `-Wl,--icf=all` to fold the
instantiations that compile to identical code (very common for same-size types);
(5) if the type only flows through, consider **type erasure** at the boundary. Measure
each step with `bloaty`; expect the thin-template refactor to recover most of it.

**B5.** A 3.3x gap between machines is an environment difference, not a code problem.
Check: core count and `-j` (is the laptop running `-j4` against CI's `-j32`?), **RAM**
(swapping, or `-flto`/LTO link exceeding memory), whether **ccache** is configured
locally (CI may have a warm shared cache), disk speed and whether the object directory is
on a network/virtualized filesystem, thermal throttling, an antivirus/indexer scanning
build output, and whether the laptop build is using a different (slower) linker or a
Debug config with different flags. Give developers the same presets
(`CMakePresets.json`), a shared remote cache (`sccache` + S3), and — often the real
answer — a **remote build** option so their laptop is not the bottleneck.

**B6.** Both numbers are expected: LTO shrinks the binary and moves all the optimization
work to link time, single-threaded in the classic implementation. Fix: use **ThinLTO**
(`-flto=thin`), which partitions the work and parallelizes it, keeping most of the benefit
for a fraction of the cost; add `-flto=auto`/`-flto=$(nproc)` for gcc's parallel LTO;
enable **LTO only in release/CI builds**, never in the developer incremental build; and
cache the link where possible. Also verify the 8% is worth it — measure performance too,
since LTO's main benefit is usually speed rather than size.

---

**C1.** Ordered plan for 40 min clean / 90 s incremental / 6 min link:
1. **Measure and split** (1 hour): `-ftime-trace` + ClangBuildAnalyzer for compile,
   `ninja -d stats` + `ninjatracing` for the critical path, and record compile-vs-link
   split. Everything below is prioritized by what this shows; do not skip it.
2. **`mold`** (`-fuse-ld=mold`, one line): 6 min link → under 1 min. **Expected: −5 min
   on clean, −60 s on incremental.** Biggest single win.
3. **`ccache`/`sccache` with a shared remote cache** + **`ninja`**: incremental and CI
   builds drop 50-80%; CI runners stop rebuilding the world.
4. **`-gsplit-dwarf`** and `-g1` for CI configs: smaller objects, faster links, less I/O.
   **Expected: another 20-30% off link.**
5. **Include-graph surgery** (days-to-weeks, highest ceiling): IWYU across the tree,
   forward declarations, split the top-10 most-included headers, PIMPL the 2-3 worst
   offenders. **Expected: 20-40% off clean build**, and — more valuable — a much smaller
   rebuild fan-out so incremental builds stop being 380 TUs.
6. **Template cost**: replace `enable_if` chains with concepts (rejected before body
   instantiation), `extern template` for the heavy instantiations, thin-template the worst
   ones, remove `<regex>`/`<iostream>` from headers. **Expected: 10-25%.**
7. **Target granularity**: split the monolithic library so a change rebuilds a subtree,
   not everything; make sure the dependency graph is shallow and parallel
   (`ninja -t graph` to find the serialization).
8. **Blunt instruments where step 5 cannot reach**: PCH for the stable third-party
   surface, unity builds for cold directories. Note the incremental-build cost.
9. **Distributed/remote execution** if the team is large enough to justify it.
10. **Modules** for new code, incrementally.
11. **Gate it**: build time as a tracked CI metric with a budget (C3), or all of the above
    decays.

**C2.** 2 MB flash, 400 KB over (20%). Cheapest first:
1. **Verify it is not a build-config mistake** (5 minutes): is the release artifact
   stripped? Is `-g` leaking in? `size -A` — if `.debug_*` is present you are done. This
   is genuinely the most common cause and it is free.
2. **Flags** (an hour, no source changes):
   `-Os` (or `-Oz` on clang) for everything that is not hot, `-ffunction-sections
   -fdata-sections -Wl,--gc-sections`, `-fvisibility=hidden
   -fvisibility-inlines-hidden`, `-Wl,--icf=all`, `-flto`, `-fno-ident
   -Wl,--build-id=none`, `--specs=nano.specs` if newlib. Measured earlier: `-Os` + gc
   sections + hidden + LTO took 172 KB → 125 KB, about **−27%**. That alone may close a
   20% gap.
3. **Attribute the rest** (`bloaty -d compileunits,symbols`, `nm --size-sort -S`) and
   act on the top 10. Typically: `<regex>` (replace with a hand-written matcher or a
   table), `<iostream>` (replace with `printf`/`std::format`/a custom logger), `std::string`
   formatting paths, and one template over-instantiated.
4. **Exceptions and RTTI** (`-fno-exceptions -fno-rtti`,
   `-fno-asynchronous-unwind-tables`): another 5-15%, but it is a **project-wide ABI
   decision** that changes how errors are reported (you need `std::expected`/status codes
   — section 10) and forbids `dynamic_cast`. Do it deliberately, not as a size hack.
5. **Code changes**: thin-template the biggest instantiations, `extern template`,
   de-duplicate near-identical functions, `[[gnu::noinline]]` on large cold functions,
   move constant tables to `.rodata` as `constexpr` data, shorten or intern log strings,
   and `-ffile-prefix-map` for `__FILE__`.
6. **Feature/config split**: not every build needs every feature — compile out the debug
   tooling, the unused sensor drivers, the offline-only code paths.
7. **Last resorts**: an executable packer / compressed image with a decompressing loader
   (costs boot time and RAM), or moving code to external storage.
Then: **add the size gate** (C3) and a per-PR `bloaty` diff, because 400 KB did not
appear in one commit.

**C3.** Gates that make silent regression impossible:
```
On every PR:
  - build the release artifact exactly as shipped (same preset, stripped)
  - record: total size, per-section sizes (size -A), and bloaty -d compileunits
  - compare against the merge-base artifact:
      * FAIL if total .text + .rodata grows > 0.5% or > 10 KB (whichever is larger)
      * WARN on any single compile unit growing > 2 KB
  - post `bloaty -d symbols base.elf -- pr.elf` as a PR comment, so the growth is
    attributed to a named symbol and the author sees it without asking
  - record clean-build and incremental-build wall time (on a DEDICATED runner, not a
    shared one, and as a median of 3 — otherwise the noise exceeds the signal)
      * FAIL if clean build time grows > 5%

On main, per commit:
  - store all of the above as a time series with a dashboard
  - alert on a 7-day trend, not just a single-commit step: the failure mode is 0.3%
    per commit for six months, which no per-PR gate catches

Release:
  - hard absolute budget: the artifact MUST be under the flash size with headroom
    (e.g. fail above 90% of 2 MB), so you find out weeks before the deadline
  - archive the unstripped binary + .dwo/.debug keyed by build-id
```
Design points to state: (1) the thresholds must be **absolute where a physical limit
exists** and relative elsewhere, because relative-only gates permit unbounded drift;
(2) the diff must be **attributed to a symbol or compile unit**, or authors cannot act on
it and will ask for the gate to be removed; (3) build-time measurement needs a dedicated,
pinned runner and repetitions, or the noise makes the gate flaky and it gets disabled —
which is the most common way these systems fail; (4) include a documented **override path**
(a label that requires a second reviewer) so a legitimate 300 KB feature is not blocked,
because a gate with no escape hatch gets circumvented.
