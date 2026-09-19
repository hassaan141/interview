// PROBLEM     Median of two sorted arrays in O(log(min(m,n))).
// EXAMPLE     [1,3] and [2] -> 2.0 ;  [1,2] and [3,4] -> 2.5
// APPROACH    Do not merge (that is O(m+n)). Instead binary search the PARTITION: choose
//             how many elements of the shorter array go into the "left half" of the
//             combined sorted order; the count from the other array is then forced. The
//             partition is correct when
//                 max(left_a, left_b) <= min(right_a, right_b)
//             Then the median is read off those four boundary values. Binary search over
//             the shorter array so the range is min(m, n).
//             Use sentinels (-inf / +inf) for the empty sides so there is no special
//             casing -- that is what makes this writable under pressure.
// COMPLEXITY  Time O(log(min(m,n))), Space O(1).
// FOLLOW-UPS  kth smallest of two sorted arrays? -> same partition idea with k instead of
//             the half. K sorted arrays? -> a min-heap, O(n log k) (section 09).
//             Why search the SHORTER array? -> it bounds the search range and guarantees
//             the derived index into the longer array stays in range.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

double find_median_sorted_arrays(const std::vector<int>& a, const std::vector<int>& b) {
    if (a.size() > b.size()) return find_median_sorted_arrays(b, a);   // search the shorter
    const std::size_t m = a.size(), n = b.size();
    if (m + n == 0) return 0.0;

    constexpr double kNegInf = -std::numeric_limits<double>::infinity();
    constexpr double kPosInf =  std::numeric_limits<double>::infinity();
    const std::size_t half = (m + n + 1) / 2;          // size of the combined left half

    std::size_t lo = 0, hi = m;                         // how many of `a` go left
    while (lo <= hi) {
        const std::size_t i = lo + (hi - lo) / 2;       // from a
        const std::size_t j = half - i;                 // from b (forced)

        const double left_a  = (i == 0) ? kNegInf : a[i - 1];
        const double right_a = (i == m) ? kPosInf : a[i];
        const double left_b  = (j == 0) ? kNegInf : b[j - 1];
        const double right_b = (j == n) ? kPosInf : b[j];

        if (left_a <= right_b && left_b <= right_a) {   // correct partition
            const double max_left = std::max(left_a, left_b);
            if ((m + n) % 2 == 1) return max_left;       // odd total: it IS the median
            const double min_right = std::min(right_a, right_b);
            return (max_left + min_right) / 2.0;
        }
        if (left_a > right_b) { if (i == 0) break; hi = i - 1; }   // take fewer from a
        else                  lo = i + 1;                          // take more from a
    }
    return 0.0;                                          // unreachable for sorted inputs
}

static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

int main() {
    assert(near(find_median_sorted_arrays({1, 3}, {2}), 2.0));
    assert(near(find_median_sorted_arrays({1, 2}, {3, 4}), 2.5));
    assert(near(find_median_sorted_arrays({}, {1}), 1.0));
    assert(near(find_median_sorted_arrays({2}, {}), 2.0));
    assert(near(find_median_sorted_arrays({}, {2, 3}), 2.5));
    assert(near(find_median_sorted_arrays({1, 2, 3, 4, 5}, {6, 7, 8}), 4.5));
    assert(near(find_median_sorted_arrays({1, 1, 1}, {1, 1, 1}), 1.0));    // all equal
    assert(near(find_median_sorted_arrays({-5, -3}, {-2, -1}), -2.5));     // negatives
    assert(near(find_median_sorted_arrays({1, 2, 3}, {4, 5, 6}), 3.5));    // disjoint
    assert(near(find_median_sorted_arrays({4, 5, 6}, {1, 2, 3}), 3.5));    // reversed
    return 0;
}
