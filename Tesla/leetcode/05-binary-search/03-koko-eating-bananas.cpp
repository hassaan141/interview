// PROBLEM     Piles of bananas, h hours. Koko eats at k bananas/hour, finishing at most
//             one pile per hour (leftovers within the hour are wasted). Find the minimum
//             integer k that finishes all piles within h hours.
// EXAMPLE     piles = [3,6,7,11], h = 8 -> 4
// APPROACH    BINARY SEARCH ON THE ANSWER. The array is not sorted and is not what you
//             search. What IS monotonic is the predicate
//                 feasible(k) = (total hours at speed k) <= h
//             which is false for small k and true for all larger k -- because eating
//             faster never takes more hours. So binary search k over [1, max_pile].
//             This is the pattern to recognise; it generalizes to capacity planning,
//             rate limiting, and "smallest buffer that meets the deadline".
// COMPLEXITY  Time O(n log(max_pile)), Space O(1). Linear scan of k would be
//             O(n * max_pile).
// FOLLOW-UPS  Ceiling division without floating point: (pile + k - 1) / k -- say this,
//             because std::ceil(double) loses precision above 2^53 (course section 01).
//             Capacity to ship packages in D days (LC 1011), split array largest sum
//             (LC 410), minimum speed to arrive on time (LC 1870) are the SAME problem.
//             Overflow: summing hours can exceed int for large inputs -> long long.

#include <algorithm>
#include <cassert>
#include <vector>

int min_eating_speed(const std::vector<int>& piles, int h) {
    if (piles.empty() || h < static_cast<int>(piles.size())) return -1;   // infeasible

    // hours(k) is non-increasing in k, so feasible(k) is monotonic: exactly what a
    // binary search needs.
    const auto feasible = [&](long long k) {
        long long hours = 0;
        for (int p : piles) hours += (p + k - 1) / k;        // ceiling division
        return hours <= h;
    };

    long long lo = 1, hi = *std::ranges::max_element(piles);  // k > max_pile never helps
    while (lo < hi) {
        const long long mid = lo + (hi - lo) / 2;
        if (feasible(mid)) hi = mid;                          // mid works; try smaller
        else               lo = mid + 1;
    }
    return static_cast<int>(lo);
}

int main() {
    assert(min_eating_speed({3, 6, 7, 11}, 8) == 4);
    assert(min_eating_speed({30, 11, 23, 4, 20}, 5) == 30);   // one pile per hour
    assert(min_eating_speed({30, 11, 23, 4, 20}, 6) == 23);
    assert(min_eating_speed({1}, 1) == 1);
    assert(min_eating_speed({1000000000}, 2) == 500000000);   // large values
    assert(min_eating_speed({}, 5) == -1);
    assert(min_eating_speed({1, 2, 3}, 2) == -1);             // fewer hours than piles
    assert(min_eating_speed({5, 5, 5}, 3) == 5);
    return 0;
}
