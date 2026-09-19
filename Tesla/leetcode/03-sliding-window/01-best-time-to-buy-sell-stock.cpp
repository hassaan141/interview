// PROBLEM     One buy and one sell, buy before sell. Maximum profit, or 0.
// EXAMPLE     [7,1,5,3,6,4] -> 5 (buy at 1, sell at 6)
// APPROACH    One pass. Track the minimum price seen so far; at each day the best profit
//             ending today is price - min_so_far. This is the degenerate sliding window:
//             `left` is implicitly "the cheapest day so far".
// COMPLEXITY  Time O(n), Space O(1). Brute force over all pairs is O(n^2).
// FOLLOW-UPS  Unlimited transactions (LC 122)? -> sum every positive daily delta, O(n).
//             At most k transactions (LC 188)? -> DP, O(nk).
//             With a cooldown (LC 309)? -> a 3-state DP.
//             Return the DAYS, not just the profit? -> track the index of the minimum.

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

int max_profit(const std::vector<int>& prices) {
    int min_price = std::numeric_limits<int>::max();
    int best = 0;
    for (int p : prices) {
        min_price = std::min(min_price, p);
        best = std::max(best, p - min_price);        // 0 if we never profit
    }
    return best;
}

// Unlimited transactions: every upward step is profit you can capture.
int max_profit_unlimited(const std::vector<int>& prices) {
    int total = 0;
    for (std::size_t i = 1; i < prices.size(); ++i)
        total += std::max(0, prices[i] - prices[i - 1]);
    return total;
}

int main() {
    assert(max_profit({7, 1, 5, 3, 6, 4}) == 5);
    assert(max_profit({7, 6, 4, 3, 1}) == 0);        // monotonically decreasing
    assert(max_profit({}) == 0);
    assert(max_profit({5}) == 0);
    assert(max_profit({1, 2}) == 1);
    assert(max_profit({2, 1}) == 0);
    assert(max_profit({3, 3, 3}) == 0);

    assert(max_profit_unlimited({7, 1, 5, 3, 6, 4}) == 7);
    assert(max_profit_unlimited({1, 2, 3, 4, 5}) == 4);
    assert(max_profit_unlimited({7, 6, 4, 3, 1}) == 0);
    return 0;
}
