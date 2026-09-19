// PROBLEM     (a) Maximum loot from a row of houses, no two adjacent.
//             (b) The houses are in a CIRCLE, so the first and last are adjacent.
// APPROACH    (a) STATE: dp[i] = the best loot considering the first i houses.
//             RECURRENCE: at house i you either rob it (and add dp[i-2]) or skip it (and
//             keep dp[i-1]):  dp[i] = max(dp[i-1], dp[i-2] + nums[i]).
//             Note the state is "over the first i", NOT "ending at i" -- that distinction
//             is what makes the recurrence a simple max instead of a search.
//             BASE: dp[0] = 0, dp[1] = nums[0].  SPACE: two variables.
//             (b) The circular case reduces to TWO linear runs: either you exclude the
//             last house or you exclude the first, and take the better. You cannot take
//             both ends, so one of the two must be excluded -- that reduction is the whole
//             trick.
// COMPLEXITY  Time O(n), Space O(1).
// FOLLOW-UPS  A tree instead of a row (LC 337)? -> the same choose/skip recurrence, but as
//             a postorder returning a pair {robbed, not_robbed}. Same idea, different
//             traversal.
//             Print WHICH houses? -> keep the full dp array and walk it back.
//             Why is greedy wrong? -> [2,1,1,2]: taking the largest first (either 2)
//             blocks the other; the answer is 4, greedy-by-value gives 3.

#include <algorithm>
#include <cassert>
#include <span>
#include <vector>

// Linear: the O(1)-space core, written over a span so the circular case can reuse it.
int rob_linear(std::span<const int> houses) {
    int skip_current = 0;                              // best not including the previous
    int best = 0;                                      // best including everything so far
    for (int value : houses) {
        const int take = skip_current + value;
        skip_current = best;
        best = std::max(best, take);
    }
    return best;
}

int rob(const std::vector<int>& houses) { return rob_linear(houses); }

// Circular: exclude the first house, or exclude the last, and take the better.
int rob_circular(const std::vector<int>& houses) {
    const std::size_t n = houses.size();
    if (n == 0) return 0;
    if (n == 1) return houses[0];                       // the one case both runs would drop
    const std::span<const int> all{houses};
    return std::max(rob_linear(all.first(n - 1)),       // drop the last
                    rob_linear(all.subspan(1)));        // drop the first
}

int main() {
    assert(rob({1, 2, 3, 1}) == 4);                     // 1 + 3
    assert(rob({2, 7, 9, 3, 1}) == 12);                 // 2 + 9 + 1
    assert(rob({}) == 0);
    assert(rob({5}) == 5);
    assert(rob({2, 1}) == 2);
    assert(rob({2, 1, 1, 2}) == 4);                     // the case greedy-by-value fails
    assert(rob({0, 0, 0}) == 0);

    assert(rob_circular({2, 3, 2}) == 3);               // cannot take both 2s
    assert(rob_circular({1, 2, 3, 1}) == 4);
    assert(rob_circular({1, 2, 3}) == 3);
    assert(rob_circular({}) == 0);
    assert(rob_circular({7}) == 7);                     // single house: no adjacency
    assert(rob_circular({1, 2}) == 2);
    return 0;
}
