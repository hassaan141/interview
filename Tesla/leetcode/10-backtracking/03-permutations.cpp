// PROBLEM     (a) All permutations of distinct numbers. (b) With duplicates, all UNIQUE
//             permutations.
// APPROACH    Two idioms worth knowing:
//             (1) SWAP IN PLACE: for each i from `start`, swap nums[start] and nums[i],
//                 recurse from start+1, swap back. No extra state, but it does NOT produce
//                 lexicographic order and it cannot dedup correctly by the usual rule.
//             (2) used[] FLAGS: iterate all positions, skip the used ones. Slightly more
//                 state, but it gives lexicographic order (if the input is sorted) and it
//                 supports the standard duplicate-skip rule.
//             For (b), sort and skip nums[i] == nums[i-1] when !used[i-1] -- that forces
//             equal elements to be consumed in a fixed left-to-right order, so each
//             multiset arrangement is generated exactly once.
// COMPLEXITY  n! permutations, O(n) to copy each -> O(n * n!) time, O(n) recursion depth.
// FOLLOW-UPS  std::next_permutation generates them iteratively in lexicographic order with
//             O(1) extra space -- say this, it is the C++ answer.
//             The kth permutation directly (LC 60)? -> factorial number system, O(n^2), no
//             enumeration at all.

#include <algorithm>
#include <cassert>
#include <vector>

// (1) Swap in place: minimal state.
static void perm_swap_dfs(std::vector<int>& nums, std::size_t start,
                          std::vector<std::vector<int>>& out) {
    if (start == nums.size()) { out.push_back(nums); return; }
    for (std::size_t i = start; i < nums.size(); ++i) {
        std::swap(nums[start], nums[i]);                 // choose
        perm_swap_dfs(nums, start + 1, out);
        std::swap(nums[start], nums[i]);                 // un-choose
    }
}

std::vector<std::vector<int>> permutations(std::vector<int> nums) {
    std::vector<std::vector<int>> out;
    perm_swap_dfs(nums, 0, out);
    return out;
}

// (2) used[] flags, with the duplicate-skip rule.
static void perm_used_dfs(const std::vector<int>& nums, std::vector<bool>& used,
                          std::vector<int>& current, std::vector<std::vector<int>>& out) {
    if (current.size() == nums.size()) { out.push_back(current); return; }
    for (std::size_t i = 0; i < nums.size(); ++i) {
        if (used[i]) continue;
        // Equal elements must be consumed left to right, or the same permutation is
        // produced once per arrangement of the equal values.
        if (i > 0 && nums[i] == nums[i - 1] && !used[i - 1]) continue;
        used[i] = true;
        current.push_back(nums[i]);
        perm_used_dfs(nums, used, current, out);
        current.pop_back();
        used[i] = false;
    }
}

std::vector<std::vector<int>> permutations_unique(std::vector<int> nums) {
    std::ranges::sort(nums);                              // required for the dedup rule
    std::vector<std::vector<int>> out;
    std::vector<bool> used(nums.size(), false);
    std::vector<int> current;
    perm_used_dfs(nums, used, current, out);
    return out;
}

// The C++ answer: iterative, lexicographic, O(1) extra space.
std::vector<std::vector<int>> permutations_stl(std::vector<int> nums) {
    std::ranges::sort(nums);
    std::vector<std::vector<int>> out;
    do { out.push_back(nums); } while (std::next_permutation(nums.begin(), nums.end()));
    return out;                                           // also handles duplicates!
}

static std::vector<std::vector<int>> canonical(std::vector<std::vector<int>> v) {
    std::ranges::sort(v);
    return v;
}

int main() {
    assert(permutations({1, 2, 3}).size() == 6);
    assert(canonical(permutations({1, 2, 3})) == permutations_stl({1, 2, 3}));
    assert(permutations({1}).size() == 1);
    assert(permutations({}).size() == 1);                 // the empty permutation
    assert(permutations({1, 2, 3, 4}).size() == 24);

    assert(permutations_unique({1, 1, 2}).size() == 3);   // not 6
    assert((permutations_unique({1, 1, 2})
            == std::vector<std::vector<int>>{{1, 1, 2}, {1, 2, 1}, {2, 1, 1}}));
    assert(permutations_unique({1, 1, 1}).size() == 1);
    assert(permutations_unique({}).size() == 1);
    assert(permutations_unique({1, 1, 2}) == permutations_stl({1, 1, 2}));
    return 0;
}
