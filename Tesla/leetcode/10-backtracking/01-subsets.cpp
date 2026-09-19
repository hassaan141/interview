// PROBLEM     (a) All subsets of a set of DISTINCT integers.  (b) With duplicates, all
//             UNIQUE subsets.
// EXAMPLE     [1,2,3] -> [[],[1],[1,2],[1,2,3],[1,3],[2],[2,3],[3]]
// APPROACH    At each index you either take the element or you do not. Recurse from
//             index i, and for each choice j >= i, take j and recurse from j+1. Record the
//             current buffer at EVERY node (not only at leaves), because every prefix is
//             itself a subset.
//             (b) Sort, then at each level skip a value equal to the previous one at the
//             SAME level -- that is what removes duplicate subsets without a hash set.
// COMPLEXITY  2^n subsets, O(n) to copy each, so O(n * 2^n) time and output space; O(n)
//             recursion depth.
// FOLLOW-UPS  Iterative? -> for each element, append it to every existing subset. Same
//             complexity, no recursion.
//             BITMASK? -> for n <= 63, iterate mask from 0 to 2^n-1 and read the set bits.
//             That is branch-free, cache friendly and trivially parallel -- the version
//             you would actually ship. Shown below.
//             Why record at every node? -> because a subset is any prefix of choices, not
//             only a full-length one.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

static void subsets_dfs(const std::vector<int>& nums, std::size_t start,
                        std::vector<int>& current, std::vector<std::vector<int>>& out) {
    out.push_back(current);                       // every node IS a subset
    for (std::size_t i = start; i < nums.size(); ++i) {
        current.push_back(nums[i]);               // choose
        subsets_dfs(nums, i + 1, current, out);   // i+1: each element at most once
        current.pop_back();                       // UN-choose
    }
}

std::vector<std::vector<int>> subsets(const std::vector<int>& nums) {
    std::vector<std::vector<int>> out;
    out.reserve(std::size_t{1} << nums.size());
    std::vector<int> current;
    current.reserve(nums.size());
    subsets_dfs(nums, 0, current, out);
    return out;
}

// With duplicates: sort, then skip a repeated value at the same level.
static void subsets_dup_dfs(const std::vector<int>& nums, std::size_t start,
                            std::vector<int>& current, std::vector<std::vector<int>>& out) {
    out.push_back(current);
    for (std::size_t i = start; i < nums.size(); ++i) {
        if (i > start && nums[i] == nums[i - 1]) continue;   // THE dedup rule
        current.push_back(nums[i]);
        subsets_dup_dfs(nums, i + 1, current, out);
        current.pop_back();
    }
}

std::vector<std::vector<int>> subsets_with_dup(std::vector<int> nums) {
    std::ranges::sort(nums);                      // required for the dedup rule
    std::vector<std::vector<int>> out;
    std::vector<int> current;
    subsets_dup_dfs(nums, 0, current, out);
    return out;
}

// Bitmask: no recursion, no branches, trivially parallel. For n <= 63.
std::vector<std::vector<int>> subsets_bitmask(const std::vector<int>& nums) {
    const std::size_t n = nums.size();
    std::vector<std::vector<int>> out;
    out.reserve(std::size_t{1} << n);
    for (std::uint64_t mask = 0; mask < (std::uint64_t{1} << n); ++mask) {
        std::vector<int> subset;
        for (std::size_t i = 0; i < n; ++i)
            if (mask & (std::uint64_t{1} << i)) subset.push_back(nums[i]);
        out.push_back(std::move(subset));
    }
    return out;
}

static std::vector<std::vector<int>> canonical(std::vector<std::vector<int>> v) {
    std::ranges::sort(v);
    return v;
}

int main() {
    assert(subsets({1, 2, 3}).size() == 8);
    assert(canonical(subsets({1, 2, 3})) == canonical(subsets_bitmask({1, 2, 3})));
    assert((canonical(subsets({1, 2})) ==
            std::vector<std::vector<int>>{{}, {1}, {1, 2}, {2}}));
    assert(subsets({}).size() == 1);              // exactly one subset: the empty one
    assert(subsets({5}).size() == 2);
    assert(subsets({1, 2, 3, 4, 5}).size() == 32);

    assert(subsets_with_dup({1, 2, 2}).size() == 6);          // not 8
    assert((canonical(subsets_with_dup({1, 2, 2})) ==
            std::vector<std::vector<int>>{{}, {1}, {1, 2}, {1, 2, 2}, {2}, {2, 2}}));
    assert(subsets_with_dup({2, 2, 2}).size() == 4);          // {}, {2}, {2,2}, {2,2,2}
    assert(subsets_with_dup({}).size() == 1);
    return 0;
}
