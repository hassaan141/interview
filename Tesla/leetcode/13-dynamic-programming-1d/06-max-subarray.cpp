// PROBLEM     Maximum sum of a contiguous subarray (at least one element).
// EXAMPLE     [-2,1,-3,4,-1,2,1,-5,4] -> 6  (the subarray [4,-1,2,1])
// APPROACH    KADANE. STATE: best[i] = the maximum sum of a subarray ENDING AT i.
//             RECURRENCE: best[i] = max(nums[i], best[i-1] + nums[i]) -- either extend the
//             previous subarray or start fresh here. The answer is the max over all i, not
//             best[n-1].
//             The "ending at i" state is the entire insight: with "the best over the first
//             i" you cannot extend, because you do not know whether the best so far
//             touches position i.
//             Space O(1): two scalars.
// COMPLEXITY  Time O(n), Space O(1). Brute force over all subarrays is O(n^2).
// FOLLOW-UPS  Return the INDICES? -> track where the current run started (below).
//             All elements negative? -> the answer is the largest single element; the
//             max(nums[i], ...) form handles it, but an implementation initialized to 0
//             gets it WRONG. That is the classic bug and the first edge case to test.
//             CIRCULAR array (LC 918)? -> max(normal Kadane, total - MINIMUM subarray),
//             with the all-negative case special-cased. Below.
//             Maximum PRODUCT (LC 152)? -> track BOTH the max and the min, because a
//             negative times a negative becomes the new max.
//             Divide and conquer? -> O(n log n); mention it, then say Kadane is better.

#include <algorithm>
#include <cassert>
#include <limits>
#include <tuple>
#include <vector>

int max_subarray(const std::vector<int>& nums) {
    if (nums.empty()) return 0;
    int best = nums[0];
    int ending_here = nums[0];
    for (std::size_t i = 1; i < nums.size(); ++i) {
        ending_here = std::max(nums[i], ending_here + nums[i]);   // extend or restart
        best = std::max(best, ending_here);
    }
    return best;
}

// With the indices, since that is the usual follow-up.
std::tuple<int, std::size_t, std::size_t> max_subarray_indices(const std::vector<int>& nums) {
    if (nums.empty()) return {0, 0, 0};
    int best = nums[0], ending_here = nums[0];
    std::size_t best_lo = 0, best_hi = 0, current_lo = 0;
    for (std::size_t i = 1; i < nums.size(); ++i) {
        if (ending_here + nums[i] < nums[i]) { ending_here = nums[i]; current_lo = i; }
        else                                   ending_here += nums[i];
        if (ending_here > best) { best = ending_here; best_lo = current_lo; best_hi = i; }
    }
    return {best, best_lo, best_hi};
}

// Circular: either the answer does not wrap (plain Kadane) or it does, in which case the
// complement is a MINIMUM subarray.
int max_subarray_circular(const std::vector<int>& nums) {
    if (nums.empty()) return 0;
    int total = 0;
    int max_end = nums[0], max_best = nums[0];
    int min_end = nums[0], min_best = nums[0];
    for (std::size_t i = 0; i < nums.size(); ++i) {
        total += nums[i];
        if (i == 0) continue;
        max_end = std::max(nums[i], max_end + nums[i]);
        max_best = std::max(max_best, max_end);
        min_end = std::min(nums[i], min_end + nums[i]);
        min_best = std::min(min_best, min_end);
    }
    if (max_best < 0) return max_best;             // ALL negative: the wrap case is invalid
    return std::max(max_best, total - min_best);
}

// Maximum product: track the min too, because a negative can become the max.
int max_product(const std::vector<int>& nums) {
    if (nums.empty()) return 0;
    int best = nums[0], cur_max = nums[0], cur_min = nums[0];
    for (std::size_t i = 1; i < nums.size(); ++i) {
        const int v = nums[i];
        const int candidates[]{v, cur_max * v, cur_min * v};
        cur_max = *std::ranges::max_element(candidates);
        cur_min = *std::ranges::min_element(candidates);
        best = std::max(best, cur_max);
    }
    return best;
}

int main() {
    assert(max_subarray({-2, 1, -3, 4, -1, 2, 1, -5, 4}) == 6);
    assert(max_subarray({1}) == 1);
    assert(max_subarray({5, 4, -1, 7, 8}) == 23);
    assert(max_subarray({-1}) == -1);                     // all negative
    assert(max_subarray({-3, -1, -2}) == -1);             // the classic bug case
    assert(max_subarray({}) == 0);
    assert(max_subarray({0, 0}) == 0);

    const auto [sum, lo, hi] = max_subarray_indices({-2, 1, -3, 4, -1, 2, 1, -5, 4});
    assert(sum == 6 && lo == 3 && hi == 6);

    assert(max_subarray_circular({1, -2, 3, -2}) == 3);
    assert(max_subarray_circular({5, -3, 5}) == 10);      // wraps around
    assert(max_subarray_circular({-3, -2, -3}) == -2);    // all negative
    assert(max_subarray_circular({3, -1, 2, -1}) == 4);

    assert(max_product({2, 3, -2, 4}) == 6);
    assert(max_product({-2, 0, -1}) == 0);
    assert(max_product({-2, 3, -4}) == 24);               // two negatives make a positive
    return 0;
}
