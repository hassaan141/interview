# 05 — Binary Search

## Recognise it

- The input is **sorted**, or rotated-sorted, or has a **monotonic predicate**.
- The required complexity is **O(log n)**.
- **"Binary search on the answer"**: the answer lives in a numeric range and there is a
  monotonic feasibility check — "can we finish in `h` hours at speed `k`?" is false for
  small `k` and true for all larger `k`. This is the version that actually shows up in
  systems interviews (capacity planning, rate limits, scheduling).

## The template you should write every time

```cpp
// Find the FIRST index where pred(i) is true, given pred is false...false,true...true.
std::size_t lo = 0, hi = n;                 // hi is EXCLUSIVE -> no empty-range trap
while (lo < hi) {
    const std::size_t mid = lo + (hi - lo) / 2;      // never lo + hi (overflow)
    if (pred(mid)) hi = mid;                         // mid might be the answer: keep it
    else           lo = mid + 1;                     // mid is definitely not
}
return lo;                                            // == n if no index satisfies pred
```
Use this *one* shape for everything: exact match (`pred = v[i] >= target`, then check
`v[lo] == target`), lower bound, upper bound (`pred = v[i] > target`), and
binary-search-on-the-answer (`pred = feasible(mid)`). Memorizing one invariant beats
memorizing four variants.

**Invariant**: `pred` is false for everything in `[0, lo)` and true for everything in
`[hi, n)`. The loop shrinks the unknown region `[lo, hi)` and terminates because it
strictly shrinks every iteration.

## In C++, say the library first

```cpp
std::ranges::lower_bound(v, x);      // first >= x
std::ranges::upper_bound(v, x);      // first > x
std::ranges::equal_range(v, x);      // both
std::ranges::binary_search(v, x);    // bool
std::ranges::partition_point(v, pred);   // THE general one: first element where pred is
                                          // false... use it for "binary search on answer"
```
Mention that `lower_bound` on a `std::list` is O(n) increments even though it does O(log n)
comparisons — the complexity guarantee is on *comparisons*, not on time.

## Problems

| File | Problem | Shape |
| --- | --- | --- |
| `01-binary-search.cpp` | Binary Search | the template itself, plus lower/upper bound |
| `02-search-2d-matrix.cpp` | Search a 2D Matrix | flatten the index |
| `03-koko-eating-bananas.cpp` | Koko Eating Bananas | **binary search on the answer** |
| `04-find-min-rotated.cpp` | Find Minimum in Rotated Sorted Array | compare against the right end |
| `05-search-rotated.cpp` | Search in Rotated Sorted Array | one half is always sorted |
| `06-median-two-sorted-arrays.cpp` | Median of Two Sorted Arrays | partition search, O(log min(m,n)) |

## Traps

1. `(lo + hi) / 2` **overflows** for large indices — use `lo + (hi - lo) / 2`.
2. Infinite loop from `lo = mid` without `+1` when the range can stop shrinking.
3. Off-by-one from mixing inclusive and exclusive `hi`. Pick exclusive and stay there.
4. `while (lo <= hi)` with `std::size_t hi = n - 1` underflows on an empty input.
5. Forgetting to verify `v[lo] == target` after a lower-bound search.
6. For "binary search on the answer", the predicate **must** be monotonic — say why it is.
