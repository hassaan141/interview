# 12 — STL: Utilities, Containers, Iterators, Algorithms, Ranges

Course chapters: **19 (Utilities), 20 (Containers, Iterators, Algorithms)**

Know the complexity table cold and know *why* `std::vector` beats `std::list`. Those
two things account for most STL interview questions.

---

## 1. Views: `std::span`, `std::string_view`, `std::mdspan`

```cpp
void process(std::span<const float> data);       // replaces (const float*, size_t)
void fill(std::span<float, 3> exactly_three);    // static extent: size in the type
std::span sub = data.subspan(2, 4);
auto first = data.first(2); auto last = data.last(2);
std::as_bytes(data);                              // span<const std::byte>

void log(std::string_view msg);                   // no allocation for a literal
sv.substr(1, 3);   sv.starts_with("pre");   sv.remove_prefix(2);
```
Both are **non-owning, trivially copyable, two words wide** (pointer + size). Never
store one that can outlive its buffer. `string_view` is **not null-terminated** —
never pass `.data()` to a C API.

`std::mdspan` (C++23) is the multidimensional view — the right way to express an
image or a tensor over caller-owned memory:
```cpp
std::mdspan<float, std::extents<std::size_t, 3, 4>> m{ptr};         // static 3x4
std::mdspan<float, std::dextents<std::size_t, 2>> img{ptr, h, w};   // dynamic
img[i, j];                                     // C++23 multi-arg subscript
// layout_right = row-major (C), layout_left = column-major (Fortran/BLAS),
// layout_stride = arbitrary strides (a sub-image / ROI without copying)
```

## 2. Container complexity — memorize this

| Container | random access | insert/erase middle | push_back | push_front | find | memory | iterator invalidation |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `array<T,N>` | O(1) | — | — | — | O(n) | inline, zero overhead | never |
| `vector<T>` | **O(1)** | O(n) | **amortized O(1)** | O(n) | O(n) | contiguous, 3 ptrs + cap slack | **all on realloc**; from the point on erase |
| `deque<T>` | O(1) (two derefs) | O(n) | O(1) | **O(1)** | O(n) | chunked (512 B blocks) | all iterators on insert; refs survive end-insertion |
| `list<T>` | **O(n)** | **O(1)** given an iterator | O(1) | O(1) | O(n) | 2 ptrs + alloc **per node** | only the erased element |
| `forward_list<T>` | O(n) | O(1) after | — | O(1) | O(n) | 1 ptr per node | only the erased |
| `set`/`map` | O(n) | O(log n) | — | — | **O(log n)** | RB-tree, 3 ptrs + color per node | only the erased |
| `unordered_set`/`map` | — | O(1) avg / O(n) worst | — | — | **O(1) avg** | buckets + node per element | all on **rehash**; refs never |
| `flat_map` (C++23) | O(1) | O(n) | — | — | O(log n) | two contiguous vectors | all on insert |

**The single most useful fact: `std::vector` beats `std::list` at almost everything,
including insertion in the middle, for anything up to thousands of small elements** —
because a linked-list traversal is a dependent pointer chase (a cache miss per node,
~100 ns) while a `memmove` runs at tens of GB/s and prefetches perfectly. Be ready to
say this with numbers; it is a favourite question.

Choosing, in practice:
- **`std::vector`** — the default. Always. `reserve()` if you know the size.
- **`std::array`** — fixed size known at compile time, no allocation.
- **`std::deque`** — stable front *and* back insertion; also the default `std::queue`
  and `std::stack` backing store.
- **`std::list`** — only for `splice`, or when you need reference stability *and*
  O(1) removal from the middle with a held iterator. Rare.
- **`std::map`** — ordered iteration, range queries, stable references.
- **`std::unordered_map`** — lookups by key, no ordering needed. Beware: node-based,
  so one allocation per element and a pointer chase per lookup.
- **`flat_map`/sorted vector** — read-mostly lookup tables; far better cache
  behaviour than either tree or hash map.
- Third-party open-addressing hash maps (`absl::flat_hash_map`) are typically 2-3x
  faster than `std::unordered_map` because the standard's requirements (reference
  stability, bucket interface) force a node-based design.

Container adaptors: `std::stack`, `std::queue` (both on `deque` by default),
`std::priority_queue` (on `vector` + heap algorithms; **max-heap** by default — use
`std::greater<>` for a min-heap).

### Invalidation rules you must state correctly
- `vector`: any reallocation invalidates **everything**; `insert`/`erase` invalidates
  from that position onward. `reserve` invalidates all. `push_back` invalidates all
  **iff** it reallocates.
- `deque`: `insert`/`erase` in the middle invalidates all iterators *and* references;
  insertion at either **end** invalidates iterators but **not references**.
- `list`/`forward_list`/associative: only iterators/references to the **erased**
  element are invalidated.
- `unordered_*`: **rehash** invalidates all *iterators* but never *references or
  pointers* (the nodes do not move).

## 3. Iterators

Categories (C++20 adds the concepts and a new `contiguous` tier):
```
input/output  →  forward  →  bidirectional  →  random_access  →  contiguous
++            ++, multi-pass  ++/--          += n, O(1) diff    data() is valid
```
`std::input_iterator`, `std::forward_iterator`, `std::bidirectional_iterator`,
`std::random_access_iterator`, `std::contiguous_iterator`.

Writing one (they ask for this):
```cpp
class Iter {
public:
    using iterator_category = std::random_access_iterator_tag;   // pre-C++20 traits
    using value_type        = int;
    using difference_type   = std::ptrdiff_t;
    using pointer           = int*;
    using reference         = int&;

    reference operator*() const { return *p_; }
    Iter& operator++()    { ++p_; return *this; }
    Iter  operator++(int) { Iter t = *this; ++p_; return t; }
    bool  operator==(const Iter&) const = default;
    // + operator--, +=, -=, +, -, [], <=> for random access
private:
    int* p_{};
};
```
Utilities: `std::advance`, `std::next`, `std::prev`, `std::distance`,
`std::iter_swap`, `std::iterator_traits<It>`. Use the free functions
`std::begin`/`std::end`/`std::size`/`std::data` (they work on C arrays too).

Special iterators worth naming: `std::back_insert_iterator`
(`std::back_inserter`), `std::ostream_iterator`, `std::istream_iterator`,
`std::reverse_iterator`, `std::move_iterator`, `std::counted_iterator`.

## 4. Algorithms

```cpp
// non-modifying
std::find, find_if, find_if_not, count, count_if, all_of, any_of, none_of,
std::equal, mismatch, search, adjacent_find, for_each
// modifying
std::copy, copy_if, copy_n, move, transform, fill, generate, replace, swap_ranges,
std::remove_if, unique, reverse, rotate, shuffle, sample
// partitioning / sorting / searching (need random access unless noted)
std::sort (O(n log n), introsort), stable_sort (O(n log² n) or O(n log n) with memory),
std::partial_sort, nth_element (O(n) average -- the right answer for "top k"),
std::partition, stable_partition, lower_bound, upper_bound, equal_range,
std::binary_search, is_sorted, merge, inplace_merge
// heap
std::make_heap, push_heap, pop_heap, sort_heap, is_heap
// numeric
std::accumulate (sequential, ordered), reduce (may reorder -> parallelizable),
std::transform_reduce, inner_product, partial_sum, inclusive_scan, exclusive_scan,
std::iota, gcd, lcm, midpoint, clamp, lerp
// min/max
std::min, max, minmax, min_element, max_element, clamp
```

**The erase-remove idiom** (and its C++20 replacement):
```cpp
v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());   // classic
std::erase_if(v, pred);                                        // C++20: do this
```
`std::remove_if` only **shuffles** the kept elements forward and returns the new
logical end — it cannot change the container's size. Forgetting the `erase` is one of
the most common STL bugs.

`std::accumulate` vs `std::reduce`: `accumulate` is strictly left-to-right sequential;
`reduce` may reorder and parallelize, so it requires associativity/commutativity — and
with floating point it gives a *different* (equally valid) answer. Also
`std::accumulate`'s init type determines the accumulator type:
`std::accumulate(v.begin(), v.end(), 0)` on a `vector<double>` truncates to `int`.

`std::nth_element` is the answer to "find the median / the top k" — O(n) average,
versus O(n log n) for a full sort.

## 5. C++20 Ranges

```cpp
#include <ranges>
namespace rv = std::views;

auto evens = v | rv::filter([](int x) { return x % 2 == 0; })
               | rv::transform([](int x) { return x * x; })
               | rv::take(5);
for (int x : evens) { }                       // LAZY: nothing computed until iterated

std::ranges::sort(v);                          // no .begin()/.end()
std::ranges::find(v, 42);
std::ranges::max_element(v, {}, &Point::x);    // projections!
std::ranges::for_each(v, print, &Frame::id);

rv::iota(0, 10), rv::reverse, rv::drop, rv::take_while, rv::split, rv::join,
rv::keys, rv::values, rv::elements<0>, rv::enumerate (C++23), rv::zip (C++23),
rv::chunk, rv::slide, rv::adjacent (C++23)
std::ranges::to<std::vector>(view)             // C++23: materialize
```
Benefits: composable, lazy (no intermediate containers), no iterator-pair
boilerplate, projections remove most one-line lambdas. Costs: compile time, error
messages, debug-build performance (many small function objects that need inlining),
and dangling-view hazards (`rv::filter` over a temporary container). Say both sides.

## 6. Utility class templates

```cpp
std::pair<A,B> p{1, 2.0};              auto [a, b] = p;
std::tuple<A,B,C> t{1, 2.0, 'c'};      std::get<0>(t); std::tuple_size_v<decltype(t)>;
std::apply(fn, t);                      std::tie(a, b) = p;   std::tuple_cat(t1, t2);
std::optional<T> o;                     o.has_value(); *o; o.value_or(d); o.reset();
                                        o.and_then(f); o.transform(f);   // C++23
std::variant<A,B,C> v;                  std::holds_alternative<A>(v); std::get_if<A>(&v);
                                        std::visit(overloaded{...}, v);
std::any a = 42;                        std::any_cast<int>(a);   // type-erased, allocates
std::expected<T,E> e;                   // C++23
```
`std::optional` is the right "maybe" type; it stores the value **inline** (no
allocation) plus a bool, so `sizeof(optional<int>)` is 8. `std::variant` stores the
largest alternative inline plus a tag — no allocation. `std::any` type-erases and
usually allocates; avoid it in hot code.

Other utilities worth knowing by name: `std::format`/`std::print` (C++20/23 — type
safe, faster than iostreams, and no locale surprises), `std::chrono` (always use
`steady_clock` for durations, `system_clock` only for wall time),
`std::random_device` + `std::mt19937` + a distribution (never `rand() % n`),
`std::filesystem`, `std::bit_cast`/`std::popcount`/`std::countl_zero`/
`std::has_single_bit` (`<bit>`), `std::source_location`.

```cpp
// timing, done right
const auto t0 = std::chrono::steady_clock::now();
work();
const auto dt = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - t0);
std::print("{} us\n", dt.count());
```

---

## Traps checklist

1. `std::remove_if` does not remove — pair it with `erase`, or use `std::erase_if`.
2. `std::vector` invalidates everything on reallocation; `reserve` up front.
3. `unordered_map` rehash invalidates iterators but **not** references.
4. `map::operator[]` **inserts** a default value and is non-`const`; use `at`/`find`.
5. `std::accumulate(v.begin(), v.end(), 0)` on doubles truncates to `int`.
6. `accumulate` is ordered; `reduce` may reorder — FP results differ.
7. `priority_queue` is a **max**-heap by default.
8. `string_view` is not null-terminated and does not own.
9. Storing a `span`/`string_view` member that outlives its buffer is a dangling bug.
10. `std::list` is almost never the right answer; `vector` wins on cache behaviour.
11. `nth_element` (O(n)) beats `sort` (O(n log n)) for medians and top-k.
12. Views are lazy — a filtered view over a temporary container dangles.
