// PROBLEM     (a) How many distinct ways to climb n stairs taking 1 or 2 steps?
//             (b) Minimum cost to reach the top when each step has a cost.
// APPROACH    STATE: dp[i] = the number of ways to reach step i.
//             RECURRENCE: the last move was either a 1-step from i-1 or a 2-step from i-2,
//             and those sets of paths are disjoint, so dp[i] = dp[i-1] + dp[i-2].
//             BASE: dp[0] = 1 (one way to stand still), dp[1] = 1.
//             That is the Fibonacci sequence, which is worth saying out loud.
//             SPACE: dp[i] reads only the previous two values, so two variables suffice --
//             O(n) -> O(1). Making that optimization unprompted is the point of the
//             problem.
// COMPLEXITY  Time O(n), Space O(1). Naive recursion is O(2^n) because it recomputes the
//             same subproblems -- that overlap IS why DP applies.
// FOLLOW-UPS  Steps of size 1..k? -> dp[i] = sum of the previous k, a sliding-window sum.
//             n up to 10^18? -> matrix exponentiation, O(log n).
//             Overflow? -> Fibonacci exceeds int at n=47 and long long at n=93. Use
//             unsigned long long, or a modulus, and SAY SO -- interviewers check.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

// O(1) space rolling version.
std::uint64_t climb_stairs(int n) {
    if (n < 0) return 0;
    std::uint64_t prev = 1, curr = 1;                 // dp[0], dp[1]
    for (int i = 2; i <= n; ++i) {
        const std::uint64_t next = prev + curr;
        prev = curr;
        curr = next;
    }
    return curr;
}

// (b) Minimum cost climbing stairs: you may start at index 0 or 1, and the "top" is
// one past the last element.
int min_cost_climbing_stairs(const std::vector<int>& cost) {
    const std::size_t n = cost.size();
    if (n == 0) return 0;
    if (n == 1) return 0;                              // you can start past it
    int two_back = 0, one_back = 0;                    // cost to reach steps i-2 and i-1
    for (std::size_t i = 2; i <= n; ++i) {
        const int current = std::min(one_back + cost[i - 1], two_back + cost[i - 2]);
        two_back = one_back;
        one_back = current;
    }
    return one_back;
}

int main() {
    assert(climb_stairs(0) == 1);                      // one way: do nothing
    assert(climb_stairs(1) == 1);
    assert(climb_stairs(2) == 2);                      // 1+1, 2
    assert(climb_stairs(3) == 3);
    assert(climb_stairs(4) == 5);
    assert(climb_stairs(10) == 89);
    assert(climb_stairs(50) == 20365011074ULL);        // would have overflowed int
    assert(climb_stairs(-1) == 0);

    assert(min_cost_climbing_stairs({10, 15, 20}) == 15);
    assert(min_cost_climbing_stairs({1, 100, 1, 1, 1, 100, 1, 1, 100, 1}) == 6);
    assert(min_cost_climbing_stairs({}) == 0);
    assert(min_cost_climbing_stairs({5}) == 0);
    assert(min_cost_climbing_stairs({1, 2}) == 1);
    assert(min_cost_climbing_stairs({0, 0, 0}) == 0);
    return 0;
}
