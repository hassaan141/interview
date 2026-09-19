# 12 — Mock interview questions

## A. Rapid fire

1. Give the complexity table for `vector`, `deque`, `list`, `map`, `unordered_map`:
   random access, middle insert, push_back/front, find.
2. Why does `std::vector` usually beat `std::list` even for middle insertion?
3. Exactly when are `vector` iterators invalidated? References?
4. What does a `deque` guarantee that a `vector` does not?
5. `unordered_map` rehash — what is invalidated and what is not? Why?
6. What is wrong with `map::operator[]`? What do you use instead?
7. `try_emplace` vs `emplace` vs `insert_or_assign`.
8. Is `std::priority_queue` a min-heap or a max-heap? How do you flip it?
9. What does `std::remove_if` actually do?
10. `std::accumulate` vs `std::reduce` — two differences.
11. What is wrong with `std::accumulate(v.begin(), v.end(), 0)` on a `vector<double>`?
12. What is `std::nth_element` for and what is its complexity?
13. `lower_bound` vs `upper_bound` vs `equal_range`.
14. `sizeof(std::span<T>)`? `sizeof(std::span<T, 3>)`? Why the difference?
15. Two dangers of `std::string_view`.
16. Name the five iterator categories.
17. What five typedefs does a pre-C++20 iterator need?
18. Why do range views compose without intermediate containers? Name a hazard.
19. What is a range *projection* and why is it useful?
20. `sizeof(std::optional<int>)`? Does `optional` allocate? Does `variant`? Does `any`?
21. Which clock for measuring a duration, and why not the other one?
22. Why never `rand() % n`?
23. Why is `absl::flat_hash_map` typically faster than `std::unordered_map`?

## B. Find the bug

**B1.**
```cpp
for (auto it = v.begin(); it != v.end(); ++it)
    if (*it < 0) v.erase(it);
```

**B2.**
```cpp
std::vector<int> v{1,2,3};
int& first = v[0];
v.push_back(4);
first = 10;
```

**B3.**
```cpp
std::vector<Frame> frames;
for (int i = 0; i < 100000; ++i) frames.push_back(make_frame());
```

**B4.**
```cpp
std::map<std::string, int> counts;
if (counts["key"] == 0) std::cout << "absent";
std::cout << counts.size();
```

**B5.**
```cpp
std::string_view name = get_config()["name"].as_string();   // returns std::string
use(name);
```

**B6.**
```cpp
auto v = std::vector{5,3,1};
v.erase(std::remove_if(v.begin(), v.end(), [](int x){ return x < 4; }));
```

**B7.**
```cpp
double mean(const std::vector<float>& v) {
    return std::accumulate(v.begin(), v.end(), 0) / v.size();
}
```

**B8.**
```cpp
auto evens = make_vector() | std::views::filter([](int x){ return x % 2 == 0; });
for (int x : evens) use(x);
```

**B9.**
```cpp
std::unordered_map<Point, int> m;    // Point is a struct with two ints
```

**B10.**
```cpp
std::vector<int> v = load();          // 10 million elements
std::sort(v.begin(), v.end());
auto it = std::find(v.begin(), v.end(), target);
```

## C. Whiteboard

**C1.** Write a `RingBuffer<T, N>` with a full random-access iterator so
`std::ranges::sort` and a range-`for` both work on it.

**C2.** You need a lookup table of ~500 sensor calibration entries, built once at
startup and then read millions of times per second. Compare `std::map`,
`std::unordered_map`, a sorted `std::vector` + `lower_bound`, and a perfect hash.
Pick one with reasoning.

**C3.** Write `top_k(span<const Detection>, k)` returning the k highest-confidence
detections. Give two implementations with different complexity and say when each
wins.

**C4.** Replace this C-style interface with STL/ranges idioms and justify each change:
```cpp
void filter_points(const Point* in, int n, Point* out, int* out_n, float min_z);
```

---
---

# Answers

**A1.**

| | random access | middle insert/erase | push_back | push_front | find |
| --- | --- | --- | --- | --- | --- |
| `vector` | O(1) | O(n) | amortized O(1) | O(n) | O(n) |
| `deque` | O(1) | O(n) | O(1) | O(1) | O(n) |
| `list` | O(n) | O(1) *given an iterator* | O(1) | O(1) | O(n) |
| `map` | O(n) | O(log n) | — | — | O(log n) |
| `unordered_map` | — | O(1) avg, O(n) worst | — | — | O(1) avg |

**A2.** Because the O(n) `memmove` for a vector insert runs at tens of GB/s with
perfect prefetching, while `std::list`'s O(1) insert requires you to *get* to the
position first — an O(n) walk of dependent pointer loads, each a potential cache miss
(~100 ns). Plus one allocation per node, 16 bytes of pointer overhead per element, and
no vectorization. Measured in `examples.cpp`: traversal alone is **~8x** faster for
`vector`. The crossover where `list` wins is at very large elements or very large n
with held iterators — which is rare.

**A3.** **Iterators**: invalidated by any reallocation (`push_back`/`emplace_back`/
`insert`/`resize`/`reserve` that grows capacity) — *all* of them; and by
`insert`/`erase` — all iterators *at or after* the modification point.
**References/pointers**: the same rules. `shrink_to_fit` and `clear` also invalidate.
Nothing is invalidated by `operator[]`, `at`, or reading.

**A4.** Stable **references** to existing elements across insertion at either **end**
(`push_back`/`push_front`), and O(1) `push_front`. Iterators are still invalidated.
That is why `std::queue` and `std::stack` default to `deque`. The cost is two
indirections per element access and worse cache behaviour on traversal.

**A5.** Rehash invalidates all **iterators** but **no references or pointers**,
because `unordered_map` is node-based: rehashing relinks the existing nodes into new
buckets without moving the elements. That is a standard requirement, and it is exactly
why the standard containers cannot use open addressing.

**A6.** It is non-`const` and it **inserts** a default-constructed value when the key
is absent — so a "lookup" silently grows the map, and it does not compile on a `const`
map. Use `find` (returns `end()`), `at` (throws `std::out_of_range`), `contains`
(C++20), or `count`.

**A7.** `emplace(k, args...)` may construct the value **even if the key exists** (and
then discard it) — which matters if the value is expensive or move-only.
`try_emplace(k, args...)` guarantees the value is only constructed when the insert
happens, and never moves from your arguments on failure. `insert_or_assign(k, v)` is
the upsert: assigns if present, inserts if not, and tells you which happened.

**A8.** A **max**-heap: `top()` is the largest. For a min-heap:
`std::priority_queue<T, std::vector<T>, std::greater<T>>`, or negate the key, or
provide a custom comparator (remember the comparator means "lower priority than").

**A9.** It **partitions**: it moves the elements you are *keeping* to the front,
preserving their relative order, and returns an iterator to the new logical end. The
elements from there to `end()` are in a valid but unspecified state, and the
container's `size()` is **unchanged** — an algorithm cannot resize a container it only
sees through iterators. You must follow with `erase(new_end, end())`, or use
`std::erase_if` (C++20).

**A10.** (1) **Order**: `accumulate` is specified to apply the operation strictly
left-to-right; `reduce` may apply it in any order and any grouping, so it requires the
operation to be associative and commutative — and therefore it can be parallelized.
(2) **Parallelism**: `reduce` takes an execution policy. Consequence: with floating
point they give **different results**, both valid (see section 01). `reduce` also
defaults its init to `T{}` whereas `accumulate` requires it.

**A11.** The init value `0` is an `int`, and `accumulate` deduces the accumulator type
from it — so every element is converted to `int` and truncated, and the result is
`int`. `std::accumulate(v.begin(), v.end(), 0.0)` fixes it. This is a silent
wrong-answer bug, not a compile error.

**A12.** It partially sorts so that the element at position `n` is the one that would
be there in a fully sorted range, everything before it is `<=` it, and everything
after is `>=` it — nothing else is ordered. **O(n) average** (introselect / quickselect
with a median-of-medians fallback), O(n) worst in practice for libstdc++. It is the
right tool for a median, a percentile, or "the k largest" (then sort just those k).

**A13.** On a sorted range: `lower_bound(v)` = first element **not less than** `v`
(i.e. `>= v`); `upper_bound(v)` = first element **greater than** `v`;
`equal_range(v)` = both, as a pair delimiting all elements equivalent to `v`. All
O(log n) comparisons — but O(n) *increments* on a non-random-access iterator, which is
why `std::lower_bound` on a `std::list` is O(n).

**A14.** `sizeof(std::span<T>)` is **two words** (pointer + size). `sizeof(std::span<T,
3>)` is **one word** — with a static extent the size is part of the type, so no
runtime storage is needed. That also makes the static-extent version better for the
optimizer, since the trip count is a compile-time constant.

**A15.** (1) It is **non-owning**, so a view into a temporary or into a `std::string`
that is later modified/destroyed dangles — the canonical bug is
`std::string_view sv = returns_string();`. (2) It is **not guaranteed
null-terminated**, so `sv.data()` must never be handed to a C API expecting a
C-string; after `substr`/`remove_suffix` it certainly is not.

**A16.** input, output, forward, bidirectional, random access — plus C++20's
**contiguous** (where `std::to_address` is valid and pointer arithmetic on the
underlying storage works).

**A17.** `iterator_category`, `value_type`, `difference_type`, `pointer`, `reference`
(either as member typedefs or via an `std::iterator_traits` specialization). C++20
replaces this with the iterator *concepts* plus `iter_value_t`/`iter_difference_t`, and
`iterator_concept` for the new tiers.

**A18.** Because a view is a lazy *adaptor object* holding the source range and the
function; the work happens in `operator++`/`operator*` as you iterate, so nothing is
materialized and the composition fuses into one loop. Hazards: a view over a
**temporary** container dangles (the C++20 `viewable_range`/`owning_view` rules catch
some but not all cases — `rv::filter` over a returned vector bound to `auto` is the
classic); some views are **not const-iterable**; `filter` caches its first iterator so
mutating the underlying range invalidates it; and debug-build performance is poor
because it relies on inlining a stack of small function objects.

**A19.** A projection is an extra callable argument that the algorithm applies to each
element before comparing: `std::ranges::sort(people, {}, &Person::age)`. It removes the
boilerplate lambda, works with pointer-to-member syntax, and keeps the comparator
orthogonal to the field selection — so `max_element(v, std::greater{}, &P::score)` reads
as what it does.

**A20.** `sizeof(std::optional<int>)` is **8** (4 for the `int`, 1 for the bool, padded
to the alignment of `int`). `optional` and `variant` **never allocate** — they store the
value inline (variant: the largest alternative plus a tag). `std::any` type-erases and
**does allocate** for anything beyond a small internal buffer, so keep it out of hot
code.

**A21.** `std::chrono::steady_clock` — it is monotonic and never adjusted, so a
difference of two readings is a real elapsed duration. `system_clock` is wall-clock
time and can jump backwards or forwards (NTP, DST, manual changes), which makes a
duration computed from it meaningless or negative. Use `system_clock` only when you
need a human-meaningful timestamp; use `high_resolution_clock` never (it is an alias
for one of the others, implementation-defined which).

**A22.** Two reasons: (1) **modulo bias** — unless `RAND_MAX + 1` is a multiple of
`n`, the low residues are more likely; (2) **quality** — `rand()` is typically a
weak LCG with poor low-order bits and a short period, and it is not thread-safe.
Use `<random>`: a seeded engine (`std::mt19937`, seeded from `std::random_device` or a
fixed seed for reproducibility) plus a distribution
(`std::uniform_int_distribution`), which handles the bias correctly.

**A23.** Because the standard's requirements force `std::unordered_map` to be
**node-based**: reference stability across rehash, a bucket interface, and support for
non-movable values. That means one allocation per element and a pointer chase per
lookup. `absl::flat_hash_map` uses **open addressing** with SIMD-scanned control
bytes: elements live in one contiguous array, a probe touches one cache line, and there
are no per-node allocations — typically 2-3x faster lookups and much less memory. The
cost is that references and pointers are invalidated on rehash.

---

**B1.** `erase` invalidates `it` (and everything after it), so `++it` on the next
iteration is UB. `erase` returns the next valid iterator — use
`it = v.erase(it);` and only `++it` in the else branch — or just
`std::erase_if(v, [](int x){ return x < 0; });`, which is also O(n) instead of O(n²).

**B2.** `push_back` may reallocate, so `first` dangles and the write is a
use-after-free (ASan catches it). Re-acquire the reference after any operation that can
grow the vector, or `reserve` first.

**B3.** No `reserve`, so the vector reallocates ~log₂(100000) ≈ 17 times, and each
reallocation moves (or, if `Frame`'s move is not `noexcept`, **copies**) every element
so far — about 200k element relocations of work that `frames.reserve(100000)` removes
entirely. Two fixes: `reserve`, and make sure `Frame`'s move constructor is `noexcept`.

**B4.** `counts["key"]` **inserts** `{"key", 0}`, so the branch is taken *and* the map
now has an entry — `size()` prints 1, not 0. Use `counts.contains("key")` or
`counts.find`.

**B5.** `as_string()` returns a `std::string` by value; the `string_view` binds to that
temporary, which dies at the end of the statement → dangling view. Store a
`std::string`, or keep the owner alive in a named variable.

**B6.** `erase` is called with **one** iterator, so it erases exactly one element (the
one at the new logical end) instead of the whole tail. The range form is required:
`v.erase(std::remove_if(...), v.end())`. Better: `std::erase_if(v, pred)`.

**B7.** Two bugs: the init `0` is an `int`, so all the `float`s are truncated and
summed as integers; and `v.size()` is `size_t`, so the division is
integer-or-unsigned-converted and the result is wrong (and UB-adjacent if the sum is
negative). Also no guard for an empty vector (division by zero). Fix:
`std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size())`, with an
empty check — and for a long sequence prefer Welford or Kahan (section 01).

**B8.** `make_vector()` returns a temporary; `auto evens = ...` keeps only the view,
and the vector is destroyed at the end of the statement → the loop iterates a dangling
range. Fix: `auto owner = make_vector(); auto evens = owner | ...;` or materialize with
`std::ranges::to<std::vector>` (C++23).

**B9.** Does not compile: there is no `std::hash<Point>` specialization. You must
provide one (and `operator==`). Points to make: specialize `std::hash<Point>` or pass a
custom hasher; combine the fields properly (`h1 ^ (h2 << 1)` is poor —
use something like boost's `hash_combine`, or `std::hash<std::uint64_t>{}((uint64_t)x
<< 32 | (uint32_t)y)`); and a bad hash silently degrades lookups to O(n).

**B10.** The `std::find` is a **linear** scan on a range you just sorted — 10 million
comparisons instead of 24. Use `std::binary_search` / `std::lower_bound`. And if you
only needed the lookup, sorting 10M elements (O(n log n)) to then do one search was
itself the wrong plan: a single `std::find` would have been cheaper. Ask what the
access pattern is.

---

**C1.**
```cpp
template <typename T, std::size_t N>
class RingBuffer {
    std::array<T, N> buf_{};
    std::size_t head_{0}, size_{0};
public:
    // A random-access iterator over the LOGICAL order, hiding the wrap.
    class iterator {
        RingBuffer* rb_{};
        std::size_t i_{};                              // logical index, 0..size_
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        iterator() = default;
        iterator(RingBuffer* rb, std::size_t i) : rb_{rb}, i_{i} {}

        reference operator*()  const { return rb_->buf_[(rb_->head_ + i_) % N]; }
        pointer   operator->() const { return &**this; }
        reference operator[](difference_type n) const { return *(*this + n); }

        iterator& operator++() { ++i_; return *this; }
        iterator  operator++(int) { auto t = *this; ++i_; return t; }
        iterator& operator--() { --i_; return *this; }
        iterator  operator--(int) { auto t = *this; --i_; return t; }
        iterator& operator+=(difference_type n) { i_ += static_cast<std::size_t>(n); return *this; }
        iterator& operator-=(difference_type n) { return *this += -n; }
        friend iterator operator+(iterator it, difference_type n) { return it += n; }
        friend iterator operator+(difference_type n, iterator it) { return it += n; }
        friend iterator operator-(iterator it, difference_type n) { return it -= n; }
        friend difference_type operator-(const iterator& a, const iterator& b) {
            return static_cast<difference_type>(a.i_) - static_cast<difference_type>(b.i_);
        }
        friend bool operator==(const iterator& a, const iterator& b) = default;
        friend auto operator<=>(const iterator& a, const iterator& b) { return a.i_ <=> b.i_; }
    };

    void push(T v) {
        if (size_ == N) { buf_[head_] = std::move(v); head_ = (head_ + 1) % N; }
        else            { buf_[(head_ + size_) % N] = std::move(v); ++size_; }
    }
    iterator begin() { return {this, 0}; }
    iterator end()   { return {this, size_}; }
    std::size_t size() const noexcept { return size_; }
    static constexpr std::size_t capacity() noexcept { return N; }
};
// then: std::ranges::sort(rb);  for (auto& x : rb) {...}
```
Points to volunteer: the iterator stores a **logical index**, not a pointer, precisely
so `end()` is representable without forming an out-of-bounds pointer (the trap fixed in
`examples.cpp`); `operator-` must be signed; `difference_type` must be signed even
though the index is unsigned; `N` a power of two lets `% N` become a mask; and a
production version would use uninitialized storage plus placement new so `T` need not
be default-constructible, and would separate the SPSC atomic-index concurrency concern
(section 11) from the container concern.

**C2.** 500 entries, built once, read millions of times per second → **read-only,
small, latency-critical**.
- **`std::map`**: a red-black tree, ~9 levels for 500 entries, each level a *dependent
  pointer load* into a separately allocated node → up to 9 cache misses (~900 ns worst
  case). Worst choice here.
- **`std::unordered_map`**: O(1) average, but node-based: one hash, one bucket-array
  load, then a pointer chase into a node → typically 2 cache misses. Better, and
  still not great.
- **Sorted `std::vector` + `lower_bound`**: one contiguous array, ~9 comparisons but
  the whole 500-entry key array is ~2-4 KB, so after the first few accesses it lives
  **entirely in L1**; the "9 cache misses" become 9 L1 hits. Branch-predictor
  unfriendly, but you can make it branchless. ~10-20 ns.
- **Perfect hash** (keys known at build time — `gperf`, or a `constexpr` minimal
  perfect hash): a single hash computation and **one** array lookup, no probing, no
  comparison chain. ~2-5 ns.

**Pick**: a **sorted `constexpr`/static array with `lower_bound`** as the default —
it is 10 lines, has zero allocation, zero startup cost if `constexpr`, perfect cache
behaviour, and is trivially testable. Move to a perfect hash (or a direct-indexed
array, if the sensor IDs are dense small integers — which they usually are, and then
the answer is just `std::array<Calibration, 512>` and an index, ~1 ns) only if
profiling says the lookup matters. Say the last part explicitly: **if the keys are
dense small integers, the right data structure is an array, and no hashing is needed
at all.** Also mention `std::flat_map` (C++23) as the standardized sorted-vector map.

**C3.**
```cpp
// (a) nth_element: O(n) average, mutates (or copies) the input. Best for large n.
std::vector<Detection> top_k_select(std::span<const Detection> in, std::size_t k) {
    k = std::min(k, in.size());
    std::vector<Detection> out(in.begin(), in.end());                    // O(n) copy
    std::nth_element(out.begin(), out.begin() + k, out.end(),
                     [](const auto& a, const auto& b) { return a.conf > b.conf; });
    out.resize(k);
    std::ranges::sort(out, std::greater<>{}, &Detection::conf);           // O(k log k)
    return out;
}

// (b) bounded min-heap: O(n log k) time, O(k) SPACE, single pass, never copies the
//     input. Best when n is huge, streaming, or does not fit in memory.
std::vector<Detection> top_k_heap(std::span<const Detection> in, std::size_t k) {
    auto worse = [](const Detection& a, const Detection& b) { return a.conf > b.conf; };
    std::priority_queue<Detection, std::vector<Detection>, decltype(worse)> heap{worse};
    for (const auto& d : in) {                     // min-heap of size k
        if (heap.size() < k)                heap.push(d);
        else if (d.conf > heap.top().conf) { heap.pop(); heap.push(d); }
    }
    std::vector<Detection> out;
    out.reserve(heap.size());
    while (!heap.empty()) { out.push_back(heap.top()); heap.pop(); }
    std::ranges::reverse(out);                     // heap pops ascending
    return out;
}
```
When each wins: **(a)** when you can afford O(n) extra space and n is a few thousand
to millions — `nth_element` is O(n) with a very small constant and good locality, so it
beats the heap for any k that is a meaningful fraction of n. **(b)** when k ≪ n (say
k=10, n=10⁶), when the data is **streaming** and you cannot hold it all, or when you
must not copy/mutate the input — O(k) memory and one pass, at the cost of a log k
heap operation per element and poor locality. Also worth saying: for a **fixed small
k** (top 3 detections) a simple insertion into a `std::array<Detection, 3>` beats both,
branch-predictably, with zero allocation — and that is what a 100 Hz loop should use.
And: `std::ranges::partial_sort_copy` does (a) without mutating the input.

**C4.**
```cpp
// Before: 5 parameters, two of them coupled, an int length that cannot express a
// large buffer, an out-parameter for the count, no error channel, and no way to tell
// whether `out` has room.
void filter_points(const Point* in, int n, Point* out, int* out_n, float min_z);

// After, caller-owned memory (the real-time form):
[[nodiscard]] std::span<Point>
filter_points(std::span<const Point> in, std::span<Point> out, float min_z) noexcept {
    const auto written = std::ranges::copy_if(
        in, out.begin(), [min_z](const Point& p) { return p.z >= min_z; });
    return out.first(static_cast<std::size_t>(written.out - out.begin()));
}

// Or, value-semantics form (allocates -- fine offline, not in a 100 Hz loop):
[[nodiscard]] std::vector<Point>
filter_points(std::span<const Point> in, float min_z) {
    auto kept = in | std::views::filter([min_z](const Point& p) { return p.z >= min_z; });
    return std::ranges::to<std::vector>(kept);      // C++23; or copy_if + back_inserter
}
```
Justification, change by change:
- `std::span<const Point> in` replaces `(const Point*, int)`: one argument that cannot
  disagree with itself, `const` documents that the input is not modified, `size()` is
  carried, accepts `vector`/`array`/C array/subrange, and `std::size_t` can express a
  buffer over 2 GB where `int` cannot.
- `std::span<Point> out` makes the **capacity** inseparable from the pointer, so
  `out_cap` cannot be stale and the function can bound-check.
- **Returning the written subspan** instead of an `int*` out-parameter: the result is
  usable directly in a range-`for` or another algorithm, and it cannot be ignored
  (`[[nodiscard]]`).
- `noexcept` on the span form, because it allocates nothing and can therefore promise
  not to throw — a real contract improvement for a real-time caller.
- Using `std::ranges::copy_if` instead of a hand-written loop: fewer index bugs, and it
  vectorizes at least as well.
- Offering **both** forms is the point: the span version lets the caller own the memory
  strategy (pool, arena, stack) which is what a foundations API must do; the `vector`
  version is nicer for offline tools. Keep a thin `extern "C"` shim with the original
  signature if C callers exist.
