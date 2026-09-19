// PROBLEM     Circular route of stations. gas[i] fuel is available at station i, and cost[i]
//             is needed to reach i+1. Find the starting index from which you can complete
//             the loop, or -1.
// APPROACH    Two facts, and the SECOND one is what makes this a real greedy problem
//             rather than a brute force:
//               1. A solution exists iff total(gas) >= total(cost). If the totals say no,
//                  no start works; if they say yes, exactly one start works.
//               2. If you start at s and run out of fuel between i and i+1, then NO index
//                  in [s, i] can be a valid start -- because every intermediate start
//                  arrives at its own beginning with a non-negative tank, so it has no
//                  more fuel available than the run from s did. So skip straight to i+1.
//             That second argument turns O(n^2) into O(n) and is the entire question.
//             Be ready to say it; it is a clean exchange-style proof.
// COMPLEXITY  Time O(n), Space O(1). Brute force is O(n^2).
// FOLLOW-UPS  Why is the answer unique when the totals allow it? -> the running-deficit
//             argument: the index just after the minimum prefix sum is the only valid start.
//             Return ALL valid starts? -> there is at most one, unless the totals are
//             exactly equal and the tank hits 0 at several points.
//             Overflow? -> sum the totals in long long for large inputs.

#include <cassert>
#include <numeric>
#include <vector>

int can_complete_circuit(const std::vector<int>& gas, const std::vector<int>& cost) {
    const long long total_gas = std::accumulate(gas.begin(), gas.end(), 0LL);
    const long long total_cost = std::accumulate(cost.begin(), cost.end(), 0LL);
    if (total_gas < total_cost) return -1;               // fact 1: impossible

    int start = 0;
    long long tank = 0;
    for (std::size_t i = 0; i < gas.size(); ++i) {
        tank += gas[i] - cost[i];
        if (tank < 0) {                                   // fact 2: skip the whole prefix
            start = static_cast<int>(i) + 1;
            tank = 0;
        }
    }
    return start;
}

int main() {
    assert(can_complete_circuit({1, 2, 3, 4, 5}, {3, 4, 5, 1, 2}) == 3);
    assert(can_complete_circuit({2, 3, 4}, {3, 4, 3}) == -1);
    assert(can_complete_circuit({5}, {4}) == 0);
    assert(can_complete_circuit({1}, {2}) == -1);
    assert(can_complete_circuit({}, {}) == 0);            // vacuously complete
    assert(can_complete_circuit({3, 3, 4}, {3, 4, 3}) == 2);
    assert(can_complete_circuit({2, 2}, {2, 2}) == 0);    // exactly enough
    assert(can_complete_circuit({0, 0, 0}, {0, 0, 0}) == 0);
    return 0;
}
