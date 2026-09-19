# Modern C++ Programming — course broken into study sections

Source: **Modern C++ Programming** by Federico Busato — 29 lectures, 2000+
slides. Slides live in this repo at
[`../../Lang/Modern-CPP-Programming/`](../../Lang/Modern-CPP-Programming/)
(PDF per chapter, plus `modern-cpp.pdf` all-in-one).

I re-ordered the 29 chapters into **16 sections** and grouped chapters that
teach the same idea. The order below is a *learning* order for an interview in
~4 weeks, not the lecture order — move semantics (ch. 21) comes before templates
(ch. 11-12) because it is far more likely to be asked and it explains half of
what the STL does.

## Section → chapter map

| # | Section folder | Course chapters | Why it matters for this role |
| --- | --- | --- | --- |
| 01 | [`01-foundations-type-system-and-arithmetic`](01-foundations-type-system-and-arithmetic/) | 1, 2, 3, 4, 5 | Integer promotion + float traps are real autonomy bugs |
| 02 | [`02-entities-control-flow-and-namespaces`](02-entities-control-flow-and-namespaces/) | 6 | `enum class`, bitfields, `union`, attributes |
| 03 | [`03-memory-pointers-references-and-const`](03-memory-pointers-references-and-const/) | 7 | **Highest-yield folder.** Stack/heap, dangling, `constexpr`, casts, alignment |
| 04 | [`04-functions-lambdas-and-preprocessor`](04-functions-lambdas-and-preprocessor/) | 8 | Overload resolution, lambda capture lifetime |
| 05 | [`05-classes-raii-and-special-members`](05-classes-raii-and-special-members/) | 9 | RAII, rule of 0/3/5, ctor/dtor ordering |
| 06 | [`06-polymorphism-operators-and-object-layout`](06-polymorphism-operators-and-object-layout/) | 10 | vtables, devirtualization, POD/standard layout |
| 07 | [`07-templates-and-metaprogramming`](07-templates-and-metaprogramming/) | 11, 12 | Foundations code *is* templates; SFINAE → concepts |
| 08 | [`08-translation-units-linkage-and-libraries`](08-translation-units-linkage-and-libraries/) | 13, 14 | ODR, `inline`, static vs. dynamic libs, ABI |
| 09 | [`09-move-semantics-and-value-categories`](09-move-semantics-and-value-categories/) | 21 | Zero-copy data paths; perfect forwarding |
| 10 | [`10-error-handling-and-smart-pointers`](10-error-handling-and-smart-pointers/) | 22 (part 1) | `noexcept`, `std::expected`, ownership |
| 11 | [`11-concurrency-and-the-memory-model`](11-concurrency-and-the-memory-model/) | 22 (part 2), 23 (memory ordering) | Autonomy stacks are pipelines of threads |
| 12 | [`12-stl-containers-iterators-and-algorithms`](12-stl-containers-iterators-and-algorithms/) | 19, 20 | `span`, `mdspan`, complexity, ranges |
| 13 | [`13-debugging-testing-and-sanitizers`](13-debugging-testing-and-sanitizers/) | 17, 18 | Posting explicitly says "write tests ... evaluation pipeline" |
| 14 | [`14-performance-architecture-and-optimization`](14-performance-architecture-and-optimization/) | 23, 24, 25 | Posting explicitly says "benchmark, characterize, optimize" |
| 15 | [`15-software-design-conventions-and-patterns`](15-software-design-conventions-and-patterns/) | 15, 16, 26, 27 | "Evangelize best software practices" |
| 16 | [`16-binary-size-and-build-time`](16-binary-size-and-build-time/) | 28, 29 | Embedded target; build time is a platform-team problem |

## What is in every section folder

```
NN-section-name/
├── README.md      ← the teaching notes: concept, code, gotchas, cheatsheet
├── examples.cpp   ← self-contained, compiles with -std=c++20, asserts pass
└── quiz.md        ← mock interview questions; answers at the bottom
```

Work a section like this:

1. Skim the matching chapter PDF (15-20 min, don't read every slide).
2. Read the section `README.md` properly (30-40 min).
3. Compile and run `examples.cpp`, then **break it on purpose** — remove a
   `&`, delete a `std::move`, change a cast — and predict the error before
   compiling. This is the fastest way to stop sucking at C++.
4. Take `quiz.md` cold, out loud, before looking at the answers.

## Compile flags to use everywhere

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -g examples.cpp
# and for anything touching memory:
g++ -std=c++20 -g -fsanitize=address,undefined examples.cpp
```

If you cannot say what `-Wconversion` is warning about, that is the exact gap
this role will probe.
