// PROBLEM     Given an array and a target, return the indices of the two numbers that
//             add to the target. Exactly one solution exists; you may not reuse an index.
// EXAMPLE     nums = [2,7,11,15], target = 9  ->  [0,1]
// APPROACH    One pass. For each nums[i], the number we need is target - nums[i]. Keep a
//             map from value -> index of everything seen so far, and check for the
//             complement BEFORE inserting the current element (which also handles the
//             "cannot reuse an index" rule for free).
// COMPLEXITY  Time O(n) average; Space O(n). The naive nested loop is O(n^2)/O(1) --
//             this is the canonical "trade memory for time".
// FOLLOW-UPS  Sorted input? -> two pointers, O(n) time O(1) space (see section 02).
//             Need all pairs? -> the map must hold a vector of indices per value.
//             Three-sum? -> sort, then fix one element and two-pointer the rest.
//             Worst-case O(1) lookup? -> unordered_map is O(n) worst; mention that a
//             hostile input can force collisions (a real DoS vector), fixed by a
//             randomly seeded hash.

#include <cassert>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

std::optional<std::pair<int, int>> two_sum(const std::vector<int>& nums, int target) {
    std::unordered_map<int, int> index_of;          // value -> index
    index_of.reserve(nums.size());                  // avoid rehashing mid-loop

    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        const int complement = target - nums[i];    // may overflow for extreme inputs
        if (auto it = index_of.find(complement); it != index_of.end())
            return std::pair{it->second, i};
        index_of.emplace(nums[i], i);               // insert AFTER the lookup
    }
    return std::nullopt;                            // no magic {-1,-1} sentinel
}

int main() {
    assert((two_sum({2, 7, 11, 15}, 9) == std::pair{0, 1}));
    assert((two_sum({3, 2, 4}, 6) == std::pair{1, 2}));
    assert((two_sum({3, 3}, 6) == std::pair{0, 1}));      // duplicate values
    assert((two_sum({-1, -2, -3}, -5) == std::pair{1, 2}));  // negatives
    assert(!two_sum({1, 2}, 100).has_value());            // no solution
    assert(!two_sum({}, 0).has_value());                  // empty
    assert(!two_sum({5}, 10).has_value());                // cannot reuse index 0
    return 0;
}
