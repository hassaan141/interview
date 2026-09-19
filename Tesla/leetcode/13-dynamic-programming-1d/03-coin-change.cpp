// PROBLEM     (a) Fewest coins summing to `amount`, or -1.  (b) Number of combinations.
// APPROACH    (a) STATE: dp[a] = the fewest coins making amount a.
//             RECURRENCE: dp[a] = 1 + min over coins c <= a of dp[a - c].
//             BASE: dp[0] = 0. Use a sentinel ABOVE any real answer (amount+1), not
//             INT_MAX -- adding 1 to INT_MAX is signed overflow, i.e. UB (course section
//             01). That sentinel choice is a real interview detail.
//             (b) STATE: ways[a] = the number of COMBINATIONS making a. The loop order is
//             everything: coins OUTSIDE, amounts inside -> combinations (order does not
//             matter). Amounts outside, coins inside -> PERMUTATIONS. Swapping those two
//             loops silently answers a different question.
// COMPLEXITY  Time O(amount * coins), Space O(amount) for both.
// FOLLOW-UPS  Why is greedy wrong? -> coins {1,3,4}, amount 6: greedy takes 4+1+1 = 3
//             coins, optimal is 3+3 = 2. Greedy IS correct for canonical systems like US
//             coins, which is why people are surprised. Saying this is the answer they
//             want.
//             Which coins were used? -> keep a parent array and walk it back.
//             Huge amount, few coins? -> this is unbounded knapsack; for very large
//             amounts there are number-theoretic shortcuts (the Frobenius problem).

#include <algorithm>
#include <cassert>
#include <vector>

// (a) Fewest coins.
int coin_change(const std::vector<int>& coins, int amount) {
    if (amount < 0) return -1;
    const int kSentinel = amount + 1;                   // above any achievable answer
    std::vector<int> dp(static_cast<std::size_t>(amount) + 1, kSentinel);
    dp[0] = 0;
    for (int a = 1; a <= amount; ++a)
        for (int c : coins)
            if (c > 0 && c <= a)
                dp[static_cast<std::size_t>(a)] =
                    std::min(dp[static_cast<std::size_t>(a)],
                             dp[static_cast<std::size_t>(a - c)] + 1);
    return dp[static_cast<std::size_t>(amount)] == kSentinel
               ? -1 : dp[static_cast<std::size_t>(amount)];
}

// (b) Number of COMBINATIONS: coins in the OUTER loop.
long long change_combinations(const std::vector<int>& coins, int amount) {
    if (amount < 0) return 0;
    std::vector<long long> ways(static_cast<std::size_t>(amount) + 1, 0);
    ways[0] = 1;
    for (int c : coins) {                               // coin OUTSIDE -> combinations
        if (c <= 0) continue;
        for (int a = c; a <= amount; ++a)
            ways[static_cast<std::size_t>(a)] += ways[static_cast<std::size_t>(a - c)];
    }
    return ways[static_cast<std::size_t>(amount)];
}

// For contrast: PERMUTATIONS (order matters) -- amounts outside, coins inside.
long long change_permutations(const std::vector<int>& coins, int amount) {
    if (amount < 0) return 0;
    std::vector<long long> ways(static_cast<std::size_t>(amount) + 1, 0);
    ways[0] = 1;
    for (int a = 1; a <= amount; ++a)                   // amount OUTSIDE -> permutations
        for (int c : coins)
            if (c > 0 && c <= a)
                ways[static_cast<std::size_t>(a)] += ways[static_cast<std::size_t>(a - c)];
    return ways[static_cast<std::size_t>(amount)];
}

int main() {
    assert(coin_change({1, 2, 5}, 11) == 3);            // 5 + 5 + 1
    assert(coin_change({2}, 3) == -1);
    assert(coin_change({1}, 0) == 0);
    assert(coin_change({}, 0) == 0);
    assert(coin_change({}, 5) == -1);
    assert(coin_change({1, 3, 4}, 6) == 2);             // greedy would say 3
    assert(coin_change({186, 419, 83, 408}, 6249) == 20);
    assert(coin_change({5}, 3) == -1);

    assert(change_combinations({1, 2, 5}, 5) == 4);     // 5, 2+2+1, 2+1+1+1, 1x5
    assert(change_combinations({2}, 3) == 0);
    assert(change_combinations({1}, 0) == 1);           // one way: take nothing
    assert(change_combinations({10}, 10) == 1);

    // The loop order really does change the answer.
    assert(change_permutations({1, 2}, 3) == 3);        // 1+1+1, 1+2, 2+1
    assert(change_combinations({1, 2}, 3) == 2);        // 1+1+1, 1+2
    return 0;
}
