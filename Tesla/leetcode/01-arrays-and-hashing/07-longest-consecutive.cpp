// PROBLEM     Length of the longest run of consecutive integers present in an unsorted
//             array. Required: O(n).
// EXAMPLE     [100,4,200,1,3,2] -> 4  (the run 1,2,3,4)
// APPROACH    Put everything in a hash set. Then walk each value, but ONLY start counting
//             at values that begin a run -- i.e. where v-1 is not in the set. That
//             guarantees each run is walked exactly once, so the total work is O(n) even
//             though there is a nested loop.
// COMPLEXITY  Time O(n) average, Space O(n). The "start only at run heads" trick is the
//             entire insight: without it the nested loop is O(n^2).
// FOLLOW-UPS  Why is a nested loop still O(n)? -> the inner loop advances through each
//             element of a run at most once across the whole algorithm (amortized).
//             Sorted alternative? -> sort and scan, O(n log n) time, O(1) space -- often
//             faster in practice for small n. Duplicates? -> the set removes them for
//             free. Streaming? -> a union-find / interval map keyed by endpoints.

#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <vector>

int longest_consecutive(const std::vector<int>& nums) {
    const std::unordered_set<int> present(nums.begin(), nums.end());
    int best = 0;
    for (int v : present) {
        if (present.contains(v - 1)) continue;      // not a run head: skip it
        int length = 1;
        while (present.contains(v + length)) ++length;
        best = std::max(best, length);
    }
    return best;
}

// O(n log n), O(1) extra: worth offering as the simpler alternative.
int longest_consecutive_sorted(std::vector<int> nums) {
    if (nums.empty()) return 0;
    std::ranges::sort(nums);
    int best = 1, run = 1;
    for (std::size_t i = 1; i < nums.size(); ++i) {
        if (nums[i] == nums[i - 1]) continue;               // skip duplicates
        run = (nums[i] == nums[i - 1] + 1) ? run + 1 : 1;
        best = std::max(best, run);
    }
    return best;
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn(std::vector<int>{100, 4, 200, 1, 3, 2}) == 4);
        assert(fn(std::vector<int>{0, 3, 7, 2, 5, 8, 4, 6, 0, 1}) == 9);
        assert(fn(std::vector<int>{}) == 0);
        assert(fn(std::vector<int>{1}) == 1);
        assert(fn(std::vector<int>{1, 1, 1}) == 1);              // duplicates
        assert(fn(std::vector<int>{5, 4, 3, 2, 1}) == 5);        // reversed
        assert(fn(std::vector<int>{-3, -2, -1, 5}) == 3);        // negatives
    };
    check([](const std::vector<int>& v) { return longest_consecutive(v); });
    check([](std::vector<int> v) { return longest_consecutive_sorted(std::move(v)); });
    return 0;
}
