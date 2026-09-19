// PROBLEM     (a) Can an array be split into two subsets with equal sums?
//             (b) The general 0/1 knapsack, for comparison.
// APPROACH    (a) Equal halves means each subset sums to total/2, so if the total is odd
//             the answer is immediately no. Otherwise: "is there a subset summing to
//             target?" -- that is 0/1 KNAPSACK with a boolean value.
//             STATE: reachable[s] = can some subset of the items processed so far sum to s?
//             RECURRENCE: reachable[s] |= reachable[s - item].
//             THE CRITICAL DETAIL: iterate s DOWNWARD. Going upward lets you use the item
//             you just added again in the same pass, which silently turns 0/1 knapsack
//             into UNBOUNDED knapsack. That loop direction is the single most asked
//             knapsack question.
// COMPLEXITY  Time O(n * target), Space O(target). Note this is PSEUDO-polynomial --
//             linear in the VALUE of the target, not in its bit length, so the problem is
//             NP-complete in general. Saying that is a strong signal.
// FOLLOW-UPS  Bitset optimization: std::bitset<N> reachable; reachable |= reachable << item;
//             does 64 states per word operation -- a ~64x constant-factor win, and
//             genuinely how you would ship it. Shown below.
//             Unbounded knapsack (reuse allowed)? -> iterate s UPWARD. Same code.
//             Return the actual subset? -> keep the full 2D table and walk it back.
//             Values as well as weights? -> the general knapsack below.

#include <algorithm>
#include <bitset>
#include <cassert>
#include <numeric>
#include <vector>

// (a) Subset-sum with a rolling boolean row. Iterate DOWNWARD.
bool can_partition(const std::vector<int>& nums) {
    const int total = std::accumulate(nums.begin(), nums.end(), 0);
    if (total % 2 != 0) return false;                       // odd: impossible
    const int target = total / 2;
    if (target < 0) return false;

    std::vector<bool> reachable(static_cast<std::size_t>(target) + 1, false);
    reachable[0] = true;                                     // the empty subset sums to 0
    for (int item : nums)
        for (int s = target; s >= item; --s)                 // DOWNWARD: each item once
            if (reachable[static_cast<std::size_t>(s - item)])
                reachable[static_cast<std::size_t>(s)] = true;
    return reachable[static_cast<std::size_t>(target)];
}

// The bitset version: 64 states per word operation. Same algorithm, ~64x the constant.
bool can_partition_bitset(const std::vector<int>& nums) {
    constexpr std::size_t kMaxSum = 20001;
    const int total = std::accumulate(nums.begin(), nums.end(), 0);
    if (total % 2 != 0 || total < 0 || static_cast<std::size_t>(total) >= kMaxSum) return false;
    std::bitset<kMaxSum> reachable;
    reachable[0] = true;
    for (int item : nums) reachable |= reachable << item;    // the whole inner loop
    return reachable[static_cast<std::size_t>(total / 2)];
}

// (b) The general 0/1 knapsack, for reference: maximize value under a weight capacity.
int knapsack_01(const std::vector<int>& weights, const std::vector<int>& values, int capacity) {
    std::vector<int> best(static_cast<std::size_t>(capacity) + 1, 0);
    for (std::size_t i = 0; i < weights.size(); ++i)
        for (int c = capacity; c >= weights[i]; --c)         // DOWNWARD -> 0/1
            best[static_cast<std::size_t>(c)] =
                std::max(best[static_cast<std::size_t>(c)],
                         best[static_cast<std::size_t>(c - weights[i])] + values[i]);
    return best[static_cast<std::size_t>(capacity)];
}

// Unbounded knapsack: the identical loop, UPWARD.
int knapsack_unbounded(const std::vector<int>& weights, const std::vector<int>& values,
                       int capacity) {
    std::vector<int> best(static_cast<std::size_t>(capacity) + 1, 0);
    for (std::size_t i = 0; i < weights.size(); ++i)
        for (int c = weights[i]; c <= capacity; ++c)          // UPWARD -> reuse allowed
            best[static_cast<std::size_t>(c)] =
                std::max(best[static_cast<std::size_t>(c)],
                         best[static_cast<std::size_t>(c - weights[i])] + values[i]);
    return best[static_cast<std::size_t>(capacity)];
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn(std::vector<int>{1, 5, 11, 5}));               // {1,5,5} and {11}
        assert(!fn(std::vector<int>{1, 2, 3, 5}));               // total 11 is odd
        assert(!fn(std::vector<int>{1}));
        assert(fn(std::vector<int>{2, 2}));
        assert(!fn(std::vector<int>{1, 1, 1}));                  // total 3 is odd
        assert(fn(std::vector<int>{}));                          // two empty halves
        assert(fn(std::vector<int>{3, 3, 3, 4, 5}));             // {3,4,5} and {3,3}... 
    };
    check([](const std::vector<int>& v) { return can_partition(v); });
    check([](const std::vector<int>& v) { return can_partition_bitset(v); });

    // 0/1 vs unbounded on the same input: the loop direction is the only difference.
    const std::vector<int> w{2, 3, 4}, v{3, 4, 5};
    assert(knapsack_01(w, v, 5) == 7);              // items 0 and 1, each once
    assert(knapsack_unbounded(w, v, 5) == 7);       // 2+3
    assert(knapsack_01(w, v, 6) == 8);              // 2 + 4
    assert(knapsack_unbounded(w, v, 6) == 9);       // 2+2+2, reuse allowed
    assert(knapsack_01({}, {}, 5) == 0);
    assert(knapsack_01(w, v, 0) == 0);
    return 0;
}
