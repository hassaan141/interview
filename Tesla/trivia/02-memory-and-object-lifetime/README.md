# 02 — Memory and Object Lifetime

The "foundational code" half of the job. These are the questions that separate someone who
writes C++ from someone who owns a library other teams build on.

---

## Questions

1. Walk through a process's virtual address space, from low addresses to high.
2. What is the difference between virtual and physical memory? What does the MMU do?
3. What is a page fault? Minor vs. major? Roughly how long does each take?
4. What are huge pages and when do they help?
5. What actually happens inside `malloc`? Why is its worst case unbounded?
6. What is memory fragmentation? Internal vs. external?
7. How does `std::vector` grow, and what is the amortized cost?
8. What is the difference between `reserve` and `resize`?
9. What is a memory arena / bump allocator? When is it the right answer?
10. What is a slab allocator? What problem does it solve that an arena does not?
11. What is a PMR (`std::pmr`) allocator and when would you reach for one?
12. What is a "sink parameter" and how do you write one?
13. What does "trivially relocatable" mean and why does the standard library care?
14. What is the small-buffer optimization? Which standard types use it?
15. How does `std::string`'s SSO interact with `std::string_view`?
16. What is the difference between `std::span` and `std::vector&` in an API?
17. When does a `std::string_view` dangle? Give three ways.
18. What is object lifetime in C++? When does it begin and end?
19. What is the difference between storage duration and lifetime?
20. What is `std::launder` for? (Name-level answer is fine.)
21. What happens to the padding bytes of a struct on copy?
22. How would you memory-map a file, and when is that better than reading it?
23. What is copy-on-write and why did `std::string` stop using it?
24. What is a memory leak vs. unbounded growth? How do you find each?
25. How do you bound the memory of a cache?
26. What is `mlockall` and why would a real-time process call it?
27. What is the stack size of a thread, and how do you change it?
28. How do you detect a stack overflow before it crashes?
29. What is `alloca` / a VLA, and why are they banned in most codebases?
30. You must pass a struct through shared memory to a process built with a different
    compiler. List every requirement.

---
---

# Answers

**1.** Low to high: the null page (deliberately unmapped so a null dereference faults),
`.text` (code, read+execute), `.rodata` (constants, read-only), `.data` (initialized
globals), `.bss` (zero-initialized globals, no file space), the **heap** growing upward
(`brk`, plus `mmap` regions for large allocations), shared libraries and mmapped files in
the middle, then the **stack** growing downward, and the kernel region at the top
(inaccessible from user mode). ASLR randomizes the bases.

**2.** Each process sees a private virtual address space; the **MMU** translates virtual to
physical addresses using page tables, with the **TLB** caching recent translations. This
gives isolation, lets the OS overcommit and share pages (copy-on-write after `fork`), and
allows a process's memory to be non-contiguous in physical RAM.

**3.** A page fault is a trap when a virtual address has no valid translation. A **minor**
fault is resolved without I/O — the page is in memory but not mapped (first touch of a
newly allocated page, a COW copy, a shared page) — roughly **1-3 µs**. A **major** fault
requires reading from disk/swap — **100 µs to milliseconds**. In a real-time loop a major
fault is a missed deadline, which is why you `mlockall` and pre-touch.

**4.** 2 MB (or 1 GB) pages instead of 4 KB. One TLB entry then covers 512x more memory, so
a large working set stops thrashing the TLB — commonly 5-20% on memory-heavy workloads.
Costs: internal fragmentation, and with `transparent_hugepage=always` the kernel may stall
your thread compacting memory, which is why RT systems use `madvise` instead.

**5.** It searches a free list or size-class bin for a suitable block, may split it, updates
metadata, and may take a per-arena lock; if nothing fits it calls `brk`/`mmap` to get more
from the kernel. The worst case is unbounded because it may walk a long free list, contend
on a lock, or make a syscall that itself can block on memory pressure. That is why
real-time code preallocates.

**6.** **External**: free memory exists but is split into pieces too small for the request
— the allocation fails or triggers a syscall despite free space. **Internal**: the
allocator rounds a request up to a size class, so the difference is wasted inside the
block. Arenas and pools avoid both by construction.

**7.** Geometric growth — libstdc++ doubles, MSVC uses 1.5x. Each growth allocates a new
buffer, relocates the elements (`memmove` if trivially copyable, else element-wise
move-if-noexcept), and frees the old. Amortized O(1) per `push_back` because the total
relocation work across n pushes is O(n). The individual growth is an O(n) latency spike —
which is why you `reserve`.

**8.** `reserve(n)` changes **capacity** only: it allocates storage but constructs no
elements, so `size()` is unchanged. `resize(n)` changes **size**: it constructs
(value-initializes) or destroys elements. `reserve` is what you want before a known number
of `push_back`s.

**9.** A single contiguous block with a bump pointer: allocation is one add, and there is no
individual free — you reset the whole arena. Right for **phase-based** work where
everything allocated during a frame dies at the end of the frame: per-frame scratch
buffers, a parser's AST, a request's temporaries. Zero fragmentation, near-zero cost,
perfectly bounded.

**10.** A slab allocates fixed-size objects from preformatted blocks, with a free list per
size class. Unlike an arena it supports **individual free and reuse**, so it fits
long-lived objects of one type with churn (frame buffers, message objects). Arenas cannot
free individually; slabs cannot serve arbitrary sizes.

**11.** `std::pmr` gives you **runtime-polymorphic** allocators, so a container's allocator
is not part of its type: `std::pmr::vector<int>` with a
`std::pmr::monotonic_buffer_resource` over a stack array allocates nothing. Reach for it
when you want arena semantics without templating your whole API on an allocator — which is
exactly the foundations-library use case.

**12.** A parameter you intend to take ownership of: take it **by value** and `std::move`
from it. `Widget(std::string name) : name_{std::move(name)} {}`. The caller decides
copy-vs-move at the call site, and there is exactly one move either way. Forgetting the
inner `std::move` is the common bug.

**13.** That moving an object and destroying the source is equivalent to `memcpy`-ing its
bytes. The library cares because `std::vector` reallocation can then relocate the whole
buffer with one `memmove` instead of n move-constructions and n destructions. It is not yet
a standard trait (P1144), but implementations use it internally for trivially copyable
types.

**14.** Storing small values inline in the object instead of on the heap. `std::string`
(~15 chars on libstdc++), `std::function` (~16 bytes of callable state), and `std::any` all
use it. The benefit is no allocation for the common small case; the cost is a larger object
and a branch on every access.

**15.** A `string_view` into an SSO string points **into the string object itself**, so
moving or copying the string invalidates the view even though no reallocation happened.
That makes SSO strings a particularly nasty source of dangling views — a view into a
heap-allocated string at least survives a move.

**16.** `std::span<const T>` accepts a `vector`, an `array`, a C array, a subrange, or a
memory-mapped buffer, and it says "I only look at these elements". `std::vector<T>&` forces
the caller to own a `vector` and implies you might resize it. Use `span` for read/write
access to a range and `vector&` only when you genuinely need to change the size.

**17.** (1) Constructed from a temporary: `std::string_view sv = returns_string();`.
(2) Stored as a member outliving its buffer. (3) The underlying string is modified,
reallocated, or (with SSO) moved. Plus: `.data()` is not guaranteed null-terminated, so
passing it to a C API reads past the end.

**18.** Lifetime begins when initialization completes (storage obtained **and** the
constructor finished) and ends when the destructor starts (or, for trivial types, when the
storage is released or reused). Using an object outside its lifetime is UB — including
reading a member of a partially constructed object from another thread.

**19.** **Storage duration** (automatic, static, thread, dynamic) is how long the *memory*
lasts. **Lifetime** is how long the *object* is valid within that memory. They differ for
placement new (storage outlives several object lifetimes), for a union member, and for
`std::optional`.

**20.** It tells the compiler that a pointer now refers to a *new* object created in
storage previously holding a different one — needed because the compiler is allowed to
assume a `const` member or a vptr has not changed. You need it after placement-new-ing over
an object with `const` or reference members. Rare, and if you think you need it, prefer
returning the pointer that placement new gave you.

**21.** They are **indeterminate** and are not guaranteed to be copied. So two objects that
compare equal can have different bytes, which is why `memcmp` on structs is a bug and why
you must `memset` before writing a struct to shared memory if anyone will hash or compare
the raw bytes.

**22.** `mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd, 0)`. Better than `read` when you
need random access to a large file, when several processes should share the same pages,
or when you want to avoid a copy into user space. Worse when the file is small, when you
need sequential streaming (readahead is simpler), or when I/O errors matter — with `mmap`
they arrive as `SIGBUS` rather than an error return.

**23.** COW shares the buffer between copies until one is modified. `std::string` abandoned
it in C++11 because it is **not thread-safe without atomics**, and the atomic refcount on
every copy cost more than SSO saves; also `operator[]` had to assume a write and un-share.
The C++11 standard effectively banned it by requiring `operator[]` to be O(1) and
non-invalidating.

**24.** A **leak** is memory you can no longer reach (LSan/valgrind find it). **Unbounded
growth** is memory you *can* still reach but never release — a cache with no eviction, a
vector you only append to, a `shared_ptr` cycle. Valgrind reports the latter as "still
reachable", which is why people miss it. Find it with **heaptrack** or `massif` (allocation
growth by call site over time) and by monitoring container sizes as metrics.

**25.** Bound the number of entries **and** the bytes: an LRU with a capacity, or evict
until the tracked byte total is under budget. Track the actual memory (not just the entry
count, since entry sizes vary), expose the size as a metric, and decide the eviction policy
explicitly. An unbounded cache is the single most common source of "it dies after four
hours".

**26.** `mlockall(MCL_CURRENT | MCL_FUTURE)` locks the process's pages into RAM so they can
never be swapped or reclaimed. A real-time process calls it (together with pre-touching its
heap) so that a page fault — potentially a millisecond — cannot occur during a control
loop.

**27.** The main thread gets `ulimit -s`, typically 8 MB on Linux. `pthread`/`std::thread`
threads get a default from the implementation (8 MB on glibc; embedded builds often
configure 512 KB). Change it with `pthread_attr_setstacksize` before creating the thread —
`std::thread` gives you no portable way, which is a real limitation worth knowing.

**28.** Guard pages turn an overflow into a `SIGSEGV`, which you can catch with a
`sigaltstack` handler to at least log it. Proactively: `-fstack-usage` and
`-Wstack-usage=N` give per-function stack usage at compile time, static analysis can bound
the worst-case depth for non-recursive code, and you can watermark the stack with a pattern
at thread start and check how much was touched. In safety-critical code, recursion is
banned so the bound is computable.

**29.** They allocate on the stack with a runtime size. Banned because the size is usually
attacker- or data-controlled, so they are a stack-overflow (and therefore
memory-corruption) vector with no failure mode — there is no way to report "not enough
stack". VLAs are also not standard C++ at all. Use a fixed-capacity buffer with a checked
bound, or the heap.

**30.** Requirements: `std::is_standard_layout_v<T>` (C-compatible offsets) and
`std::is_trivially_copyable_v<T>` (memcpy-able); fixed-width types from `<cstdint>` only —
no `int`, `long`, `size_t`, `bool`, or `enum` without a fixed underlying type; no
pointers or references (addresses are meaningless in another process) and no virtuals; an
explicit `alignas`; padding made explicit (and zeroed, since padding bytes are
indeterminate); a version field; and a documented byte order. Enforce with
`static_assert` on the traits, `sizeof`, `alignof`, and `offsetof` of every field so any
layout change fails the build — see `cpp-course/01/quiz.md` C5 for the code.
