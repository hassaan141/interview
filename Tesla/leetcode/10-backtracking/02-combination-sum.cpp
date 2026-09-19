// PROBLEM     (a) Candidates may be reused without limit; find all combinations summing
//             to target. (b) Each candidate used at most once, input may have duplicates,
//             output must be unique.
// EXAMPLE     (a) candidates=[2,3,6,7], target=7 -> [[2,2,3],[7]]
// APPROACH    Standard backtracking. The ONE difference between (a) and (b) is the index
//             passed to the recursive call:
//                 (a) recurse from i    -> the same element may be chosen again
//                 (b) recurse from i+1  -> each element is used at most once
//             Plus, for (b), sort and skip duplicates at the same level.
//             PRUNING is what makes this fast: sort ascending, and once
//             candidates[i] > remaining, every later candidate is too, so BREAK (not
//             continue). That single line changes the practical runtime enormously.
// COMPLEXITY  Exponential in the worst case: O(n^(target/min_candidate)). The pruning does
//             not change the bound but dominates real performance.
// FOLLOW-UPS  Just the COUNT of combinations? -> that is unbounded knapsack DP, O(n*target)
//             (section 14) -- enumerate only when you truly need the combinations.
//             Negative candidates? -> the problem becomes infinite without extra
//             constraints; say so rather than silently looping.

#include <algorithm>
#include <cassert>
#include <vector>

// (a) Unlimited reuse.
static void combo_dfs(const std::vector<int>& candidates, int remaining, std::size_t start,
                      std::vector<int>& current, std::vector<std::vector<int>>& out) {
    if (remaining == 0) { out.push_back(current); return; }
    for (std::size_t i = start; i < candidates.size(); ++i) {
        if (candidates[i] > remaining) break;              // sorted: PRUNE the whole tail
        current.push_back(candidates[i]);
        combo_dfs(candidates, remaining - candidates[i], i, current, out);   // i: reuse
        current.pop_back();
    }
}

std::vector<std::vector<int>> combination_sum(std::vector<int> candidates, int target) {
    std::ranges::sort(candidates);                          // enables the break-prune
    std::vector<std::vector<int>> out;
    std::vector<int> current;
    combo_dfs(candidates, target, 0, current, out);
    return out;
}

// (b) Each element once, duplicates in the input, unique output.
static void combo2_dfs(const std::vector<int>& candidates, int remaining, std::size_t start,
                       std::vector<int>& current, std::vector<std::vector<int>>& out) {
    if (remaining == 0) { out.push_back(current); return; }
    for (std::size_t i = start; i < candidates.size(); ++i) {
        if (candidates[i] > remaining) break;
        if (i > start && candidates[i] == candidates[i - 1]) continue;   // dedup
        current.push_back(candidates[i]);
        combo2_dfs(candidates, remaining - candidates[i], i + 1, current, out);  // i+1
        current.pop_back();
    }
}

std::vector<std::vector<int>> combination_sum_ii(std::vector<int> candidates, int target) {
    std::ranges::sort(candidates);
    std::vector<std::vector<int>> out;
    std::vector<int> current;
    combo2_dfs(candidates, target, 0, current, out);
    return out;
}

int main() {
    assert((combination_sum({2, 3, 6, 7}, 7)
            == std::vector<std::vector<int>>{{2, 2, 3}, {7}}));
    assert((combination_sum({2, 3, 5}, 8)
            == std::vector<std::vector<int>>{{2, 2, 2, 2}, {2, 3, 3}, {3, 5}}));
    assert(combination_sum({2}, 1).empty());
    assert(combination_sum({}, 5).empty());
    assert(combination_sum({1}, 0).size() == 1);           // one empty combination
    assert(combination_sum({7}, 7).size() == 1);

    assert((combination_sum_ii({10, 1, 2, 7, 6, 1, 5}, 8)
            == std::vector<std::vector<int>>{{1, 1, 6}, {1, 2, 5}, {1, 7}, {2, 6}}));
    assert((combination_sum_ii({2, 5, 2, 1, 2}, 5)
            == std::vector<std::vector<int>>{{1, 2, 2}, {5}}));
    assert(combination_sum_ii({1, 1}, 3).empty());
    return 0;
}
