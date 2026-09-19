// PROBLEM     Find all UNIQUE triples that sum to zero.
// EXAMPLE     [-1,0,1,2,-1,-4] -> [[-1,-1,2],[-1,0,1]]
// APPROACH    Sort. Fix the first element i, then two-pointer over the rest for the pair
//             summing to -nums[i]. Sorting is what makes both the two-pointer scan and
//             the duplicate handling possible.
//             Duplicate control is the whole difficulty: skip repeated values at i, and
//             after recording a triple, skip repeated values at BOTH lo and hi.
// COMPLEXITY  Time O(n^2) (n iterations of an O(n) scan), Space O(1) beyond the output
//             (plus O(n) if you may not mutate the input). Brute force is O(n^3).
// FOLLOW-UPS  Early exit: once nums[i] > 0 the remaining triple can never sum to zero.
//             kSum? -> recurse: k-sum reduces to (k-1)-sum with an outer loop, O(n^(k-1)).
//             3Sum Closest? -> same scan, track the minimum |sum - target|.
//             Without sorting? -> a hash set per i, still O(n^2) but O(n) space and much
//             harder to deduplicate.

#include <algorithm>
#include <array>
#include <cassert>
#include <vector>

std::vector<std::array<int, 3>> three_sum(std::vector<int> nums) {
    std::ranges::sort(nums);
    std::vector<std::array<int, 3>> out;
    const std::size_t n = nums.size();

    for (std::size_t i = 0; i + 2 < n; ++i) {
        if (nums[i] > 0) break;                          // sorted: no triple can be 0 now
        if (i > 0 && nums[i] == nums[i - 1]) continue;   // skip duplicate first elements

        std::size_t lo = i + 1, hi = n - 1;
        while (lo < hi) {
            const long long sum = static_cast<long long>(nums[i]) + nums[lo] + nums[hi];
            if (sum < 0)      ++lo;
            else if (sum > 0) --hi;
            else {
                out.push_back({nums[i], nums[lo], nums[hi]});
                ++lo;
                --hi;
                // Skip duplicates at BOTH ends, or the same triple is emitted again.
                while (lo < hi && nums[lo] == nums[lo - 1]) ++lo;
                while (lo < hi && nums[hi] == nums[hi + 1]) --hi;
            }
        }
    }
    return out;
}

int main() {
    using T = std::array<int, 3>;
    assert((three_sum({-1, 0, 1, 2, -1, -4}) == std::vector<T>{{-1, -1, 2}, {-1, 0, 1}}));
    assert(three_sum({0, 1, 1}).empty());
    assert((three_sum({0, 0, 0}) == std::vector<T>{{0, 0, 0}}));
    assert((three_sum({0, 0, 0, 0}) == std::vector<T>{{0, 0, 0}}));   // dedup
    assert(three_sum({}).empty());
    assert(three_sum({1, 2}).empty());
    assert(three_sum({1, 2, 3}).empty());                             // all positive
    assert((three_sum({-2, 0, 1, 1, 2}) == std::vector<T>{{-2, 0, 2}, {-2, 1, 1}}));
    return 0;
}
