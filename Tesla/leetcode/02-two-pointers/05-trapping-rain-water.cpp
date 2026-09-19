// PROBLEM     Given an elevation map, compute how much rain water is trapped.
// EXAMPLE     [0,1,0,2,1,0,1,3,2,1,2,1] -> 6
// APPROACH    Water above bar i = min(max height to its left, max height to its right)
//             - height[i]. The two-pointer trick: maintain left_max and right_max while
//             converging. Whichever side has the SMALLER running max is the side whose
//             answer is already determined -- because the other side is guaranteed to
//             have something at least that tall, so the min() is fixed.
// COMPLEXITY  Time O(n), Space O(1). The prefix/suffix-array version is O(n) time and
//             O(n) space; the stack version is also O(n)/O(n). Offer the O(1) one.
// FOLLOW-UPS  Why is the smaller side safe to resolve? -> if left_max <= right_max, then
//             min(left_max, true_right_max) == left_max regardless of what is further
//             right, because true_right_max >= right_max >= left_max.
//             2D version (LeetCode 407)? -> a min-heap over the boundary, flood inward.

#include <algorithm>
#include <cassert>
#include <vector>

int trap(const std::vector<int>& height) {
    if (height.size() < 3) return 0;
    std::size_t lo = 0, hi = height.size() - 1;
    int left_max = height[lo], right_max = height[hi], total = 0;

    while (lo < hi) {
        if (left_max <= right_max) {
            ++lo;
            left_max = std::max(left_max, height[lo]);
            total += left_max - height[lo];          // never negative, by construction
        } else {
            --hi;
            right_max = std::max(right_max, height[hi]);
            total += right_max - height[hi];
        }
    }
    return total;
}

// The O(n)-space version, for comparison. Often easier to derive under pressure.
int trap_prefix_suffix(const std::vector<int>& h) {
    const std::size_t n = h.size();
    if (n < 3) return 0;
    std::vector<int> left(n), right(n);
    left[0] = h[0];
    for (std::size_t i = 1; i < n; ++i) left[i] = std::max(left[i - 1], h[i]);
    right[n - 1] = h[n - 1];
    for (std::size_t i = n - 1; i-- > 0;) right[i] = std::max(right[i + 1], h[i]);
    int total = 0;
    for (std::size_t i = 0; i < n; ++i) total += std::min(left[i], right[i]) - h[i];
    return total;
}

int main() {
    const std::vector<std::vector<int>> cases{
        {0, 1, 0, 2, 1, 0, 1, 3, 2, 1, 2, 1},
        {4, 2, 0, 3, 2, 5},
        {}, {1}, {1, 2}, {3, 2, 1}, {1, 2, 3}, {2, 0, 2}, {5, 5, 5},
    };
    const std::vector<int> expected{6, 9, 0, 0, 0, 0, 0, 2, 0};
    for (std::size_t i = 0; i < cases.size(); ++i) {
        assert(trap(cases[i]) == expected[i]);
        assert(trap_prefix_suffix(cases[i]) == expected[i]);
    }
    return 0;
}
