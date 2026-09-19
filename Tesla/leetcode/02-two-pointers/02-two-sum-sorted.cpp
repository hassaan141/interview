// PROBLEM     The input array is SORTED. Return the 1-based indices of the two numbers
//             adding to target. Use O(1) extra space.
// EXAMPLE     numbers = [2,7,11,15], target = 9 -> [1,2]
// APPROACH    One pointer at each end. If the sum is too small the only way to increase
//             it is to move `lo` right; if too large, move `hi` left. Each move discards
//             exactly the pairs that cannot work, so nothing is skipped.
// COMPLEXITY  Time O(n), Space O(1). The hash-map version (section 01) is also O(n) time
//             but O(n) space -- sortedness is what buys you the space back.
// FOLLOW-UPS  Prove it: if nums[lo]+nums[hi] < target, then nums[lo] paired with ANY
//             j <= hi is also < target, so lo can never be part of a solution with
//             anything still in range -- discarding it is safe.
//             Unsorted? -> hash map, or sort first if you may (O(n log n)).
//             Overflow? -> long long for the sum, or compare against target - nums[hi].

#include <cassert>
#include <optional>
#include <utility>
#include <vector>

std::optional<std::pair<int, int>> two_sum_sorted(const std::vector<int>& nums, int target) {
    if (nums.size() < 2) return std::nullopt;
    std::size_t lo = 0, hi = nums.size() - 1;
    while (lo < hi) {
        const long long sum = static_cast<long long>(nums[lo]) + nums[hi];   // no overflow
        if (sum == target) return std::pair{static_cast<int>(lo) + 1, static_cast<int>(hi) + 1};
        if (sum < target) ++lo;                     // only way to increase the sum
        else              --hi;                     // only way to decrease it
    }
    return std::nullopt;
}

int main() {
    assert((two_sum_sorted({2, 7, 11, 15}, 9) == std::pair{1, 2}));
    assert((two_sum_sorted({2, 3, 4}, 6) == std::pair{1, 3}));
    assert((two_sum_sorted({-1, 0}, -1) == std::pair{1, 2}));
    assert(!two_sum_sorted({1, 2, 3}, 100).has_value());
    assert(!two_sum_sorted({5}, 5).has_value());     // need two elements
    assert(!two_sum_sorted({}, 0).has_value());
    assert((two_sum_sorted({1, 1}, 2) == std::pair{1, 2}));
    // Values near INT_MAX: the long long sum is what keeps this correct.
    assert(!two_sum_sorted({2147483646, 2147483647}, 0).has_value());
    return 0;
}
