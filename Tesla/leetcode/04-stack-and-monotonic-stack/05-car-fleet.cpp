// PROBLEM     Cars at positions with speeds drive toward a target. A faster car catches a
//             slower one and they become one fleet moving at the slower speed. How many
//             fleets arrive?
// EXAMPLE     target=12, position=[10,8,0,5,3], speed=[2,4,1,1,3] -> 3
// APPROACH    Sort cars by position DESCENDING (closest to the target first) and compute
//             each car's solo arrival time (target - position) / speed. Walk from the
//             front: if a car's arrival time is <= the current fleet leader's, it catches
//             up and joins; otherwise it is a NEW fleet and becomes the new leader.
//             The "stack" here is really just the running maximum arrival time.
// COMPLEXITY  Time O(n log n) (dominated by the sort), Space O(n).
// FOLLOW-UPS  Why sort descending? -> a car can only be blocked by cars AHEAD of it.
//             Why <= and not <? -> arriving at the same time means they merge.
//             Floating point? -> comparing times as doubles is fine here, but for exact
//             behaviour compare cross-multiplied integers:
//             (target - p_i) * s_j vs (target - p_j) * s_i. Say this -- it is the kind of
//             numerical-robustness point this role cares about (course section 01).
//             Car Fleet II (LC 1776)? -> genuinely needs a monotonic stack of collisions.

#include <algorithm>
#include <cassert>
#include <numeric>
#include <vector>

int car_fleet(int target, const std::vector<int>& position, const std::vector<int>& speed) {
    const std::size_t n = position.size();
    if (n == 0) return 0;

    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), 0u);
    std::ranges::sort(order, [&](std::size_t a, std::size_t b) {
        return position[a] > position[b];              // closest to the target first
    });

    int fleets = 0;
    double slowest_arrival = -1.0;                     // the current fleet leader's time
    for (std::size_t idx : order) {
        const double arrival = static_cast<double>(target - position[idx]) / speed[idx];
        if (arrival > slowest_arrival) {               // cannot catch the fleet ahead
            ++fleets;
            slowest_arrival = arrival;
        }
        // else: it catches up and is absorbed; the leader's time is unchanged
    }
    return fleets;
}

int main() {
    assert(car_fleet(12, {10, 8, 0, 5, 3}, {2, 4, 1, 1, 3}) == 3);
    assert(car_fleet(10, {3}, {3}) == 1);
    assert(car_fleet(100, {0, 2, 4}, {4, 2, 1}) == 1);      // all merge
    assert(car_fleet(10, {6, 8}, {3, 2}) == 2);
    assert(car_fleet(10, {0, 4, 2}, {2, 1, 3}) == 1);
    assert(car_fleet(10, {}, {}) == 0);
    assert(car_fleet(10, {0, 5}, {1, 1}) == 2);             // same speed, never merge
    return 0;
}
