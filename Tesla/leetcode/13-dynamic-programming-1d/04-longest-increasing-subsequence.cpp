// PROBLEM     Length of the longest strictly increasing subsequence (not contiguous).
// EXAMPLE     [10,9,2,5,3,7,101,18] -> 4  (2,3,7,101)
// APPROACH    (a) O(n^2) DP. STATE: dp[i] = the length of the LIS ENDING AT i. Note the
//             "ending at" -- with "over the first i" you cannot extend, because you would
//             not know the last value. RECURRENCE: dp[i] = 1 + max(dp[j]) over j < i with
//             nums[j] < nums[i]. The answer is the max over all dp[i], not dp[n-1].
//             (b) O(n log n) PATIENCE SORTING. Keep `tails`, where tails[k] is the
//             SMALLEST possible tail of an increasing subsequence of length k+1. For each
//             value, binary-search its position and overwrite. `tails` is always sorted,
//             which is what makes the search valid.
//             IMPORTANT: tails is NOT itself a valid subsequence -- only its LENGTH is
//             the answer. Volunteering that shows you understand it rather than
//             memorized it.
// COMPLEXITY  (a) O(n^2)/O(n).  (b) O(n log n)/O(n).
// FOLLOW-UPS  Non-decreasing (allow equals)? -> upper_bound instead of lower_bound.
//             RECONSTRUCT the subsequence? -> keep a parent index per element and the
//             position each value took in tails; walk it back.
//             Number of LIS (LC 673)? -> carry a count alongside each dp[i].
//             Russian doll envelopes (LC 354)? -> sort by width, then LIS on height, with
//             a descending tiebreak so equal widths cannot chain.

#include <algorithm>
#include <cassert>
#include <vector>

// (a) O(n^2): the one to derive first.
int lis_quadratic(const std::vector<int>& nums) {
    if (nums.empty()) return 0;
    std::vector<int> dp(nums.size(), 1);                 // every element alone is length 1
    int best = 1;
    for (std::size_t i = 1; i < nums.size(); ++i) {
        for (std::size_t j = 0; j < i; ++j)
            if (nums[j] < nums[i]) dp[i] = std::max(dp[i], dp[j] + 1);
        best = std::max(best, dp[i]);
    }
    return best;
}

// (b) O(n log n): patience sorting.
int lis_patience(const std::vector<int>& nums) {
    std::vector<int> tails;                              // tails[k] = smallest tail of an
    tails.reserve(nums.size());                          // increasing subsequence of len k+1
    for (int v : nums) {
        // lower_bound: strictly increasing. upper_bound would allow equal values.
        const auto it = std::ranges::lower_bound(tails, v);
        if (it == tails.end()) tails.push_back(v);       // extends the longest so far
        else                   *it = v;                   // a smaller tail for that length
    }
    return static_cast<int>(tails.size());
}

// Reconstruction, since the interviewer always asks. O(n log n).
std::vector<int> lis_sequence(const std::vector<int>& nums) {
    if (nums.empty()) return {};
    std::vector<int> tails;                              // values
    std::vector<std::size_t> tail_index;                 // index in nums of each tail
    std::vector<std::size_t> parent(nums.size(), static_cast<std::size_t>(-1));

    for (std::size_t i = 0; i < nums.size(); ++i) {
        const auto it = std::ranges::lower_bound(tails, nums[i]);
        const auto pos = static_cast<std::size_t>(it - tails.begin());
        if (pos > 0) parent[i] = tail_index[pos - 1];    // the element before us
        if (it == tails.end()) { tails.push_back(nums[i]); tail_index.push_back(i); }
        else                   { *it = nums[i]; tail_index[pos] = i; }
    }
    std::vector<int> out;
    for (std::size_t i = tail_index.back(); i != static_cast<std::size_t>(-1); i = parent[i])
        out.push_back(nums[i]);
    std::ranges::reverse(out);
    return out;
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn(std::vector<int>{10, 9, 2, 5, 3, 7, 101, 18}) == 4);
        assert(fn(std::vector<int>{0, 1, 0, 3, 2, 3}) == 4);
        assert(fn(std::vector<int>{7, 7, 7, 7}) == 1);            // strictly increasing
        assert(fn(std::vector<int>{}) == 0);
        assert(fn(std::vector<int>{5}) == 1);
        assert(fn(std::vector<int>{5, 4, 3, 2, 1}) == 1);         // decreasing
        assert(fn(std::vector<int>{1, 2, 3, 4, 5}) == 5);         // already increasing
        assert(fn(std::vector<int>{-1, -2, 0}) == 2);
    };
    check([](const std::vector<int>& v) { return lis_quadratic(v); });
    check([](const std::vector<int>& v) { return lis_patience(v); });

    const auto seq = lis_sequence({10, 9, 2, 5, 3, 7, 101, 18});
    assert(seq.size() == 4);
    assert(std::ranges::is_sorted(seq) && std::ranges::adjacent_find(seq) == seq.end());
    assert(lis_sequence({}).empty());
    assert((lis_sequence({3}) == std::vector<int>{3}));
    return 0;
}
