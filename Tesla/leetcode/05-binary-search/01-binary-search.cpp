// PROBLEM     Return the index of target in a sorted array, or -1.
// APPROACH    The one template: half-open range [lo, hi), find the first index where
//             v[i] >= target, then verify. Everything else in this section is this loop
//             with a different predicate.
// COMPLEXITY  Time O(log n), Space O(1). ~20 iterations for a million elements.
// FOLLOW-UPS  Duplicates? -> lower_bound gives the first, upper_bound the past-the-end.
//             Why is binary search on a std::list O(n)? -> O(log n) COMPARISONS but O(n)
//             iterator increments; the guarantee is on comparisons.
//             Branchless / cache-friendly variants? -> Eytzinger layout or a
//             conditional-move version; for a hot lookup table this matters more than the
//             asymptotics (course section 14).

#include <algorithm>
#include <cassert>
#include <vector>

// The general primitive: first index where pred(i) is true (pred is false then true).
template <typename Pred>
std::size_t first_true(std::size_t n, Pred pred) {
    std::size_t lo = 0, hi = n;                       // hi EXCLUSIVE
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;   // overflow-safe
        if (pred(mid)) hi = mid;                      // mid may be the answer
        else           lo = mid + 1;                  // mid is not
    }
    return lo;                                        // n if pred is never true
}

int binary_search(const std::vector<int>& v, int target) {
    const std::size_t i = first_true(v.size(), [&](std::size_t m) { return v[m] >= target; });
    return (i < v.size() && v[i] == target) ? static_cast<int>(i) : -1;
}

std::size_t lower_bound_idx(const std::vector<int>& v, int target) {
    return first_true(v.size(), [&](std::size_t m) { return v[m] >= target; });
}
std::size_t upper_bound_idx(const std::vector<int>& v, int target) {
    return first_true(v.size(), [&](std::size_t m) { return v[m] > target; });
}

int main() {
    const std::vector<int> v{-1, 0, 3, 5, 9, 12};
    assert(binary_search(v, 9) == 4);
    assert(binary_search(v, 2) == -1);
    assert(binary_search(v, -1) == 0);                 // first element
    assert(binary_search(v, 12) == 5);                 // last element
    assert(binary_search(v, -100) == -1);              // below the range
    assert(binary_search(v, 100) == -1);               // above the range
    assert(binary_search({}, 1) == -1);                // empty
    assert(binary_search({5}, 5) == 0);

    const std::vector<int> dup{1, 2, 2, 2, 3};
    assert(lower_bound_idx(dup, 2) == 1);
    assert(upper_bound_idx(dup, 2) == 4);
    assert(upper_bound_idx(dup, 2) - lower_bound_idx(dup, 2) == 3);   // the count
    assert(lower_bound_idx(dup, 0) == 0);
    assert(lower_bound_idx(dup, 99) == dup.size());

    // Agrees with the standard library, which is what you should actually call.
    assert(lower_bound_idx(dup, 2)
           == static_cast<std::size_t>(std::ranges::lower_bound(dup, 2) - dup.begin()));
    return 0;
}
