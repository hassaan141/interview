// PROBLEM     Minimum number of intervals to REMOVE so that the rest do not overlap.
// EXAMPLE     [[1,2],[2,3],[3,4],[1,3]] -> 1 (remove [1,3])
// APPROACH    Equivalently: keep the MAXIMUM number of non-overlapping intervals; the
//             removals are the rest. This is the classic activity-selection problem, and
//             the greedy is: SORT BY END TIME and always keep the interval that finishes
//             earliest among those that still fit.
//             THE PROOF (exchange argument -- say this out loud): let G be the interval
//             with the earliest end. Take any optimal solution O. If O does not contain G,
//             replace O's first interval with G. G ends no later, so it cannot conflict
//             with anything else in O, and |O| is unchanged. So there is an optimal
//             solution containing G; induct on the rest.
//             Sorting by START is the natural instinct and it is WRONG: one long interval
//             starting first would block everything after it.
// COMPLEXITY  Time O(n log n), Space O(1) beyond the sort.
// FOLLOW-UPS  Weighted intervals (maximize total value)? -> the greedy FAILS; it becomes a
//             DP with binary search, O(n log n). Knowing where greedy stops being correct
//             is the real test.
//             Minimum arrows to burst balloons (LC 452)? -> the identical algorithm.
//             This is scheduling: "run the most tasks on one machine" -- directly relevant.

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <vector>

using Interval = std::array<int, 2>;                  // {start, end}, half-open [s, e)

int erase_overlap_intervals(std::vector<Interval> intervals) {
    if (intervals.size() < 2) return 0;
    std::ranges::sort(intervals, {}, [](const Interval& i) { return i[1]; });   // BY END

    int kept = 0;
    int last_end = std::numeric_limits<int>::min();
    for (const Interval& v : intervals)
        if (v[0] >= last_end) {                        // >= : touching does NOT overlap
            ++kept;
            last_end = v[1];
        }
    return static_cast<int>(intervals.size()) - kept;
}

// Minimum arrows to burst balloons: the same algorithm, counting the keepers.
int find_min_arrow_shots(std::vector<Interval> balloons) {
    if (balloons.empty()) return 0;
    std::ranges::sort(balloons, {}, [](const Interval& i) { return i[1]; });
    int arrows = 0;
    long long last = std::numeric_limits<long long>::min();
    for (const Interval& b : balloons)
        if (b[0] > last) { ++arrows; last = b[1]; }    // > : here touching DOES count
    return arrows;
}

int main() {
    assert(erase_overlap_intervals({{1, 2}, {2, 3}, {3, 4}, {1, 3}}) == 1);
    assert(erase_overlap_intervals({{1, 2}, {1, 2}, {1, 2}}) == 2);
    assert(erase_overlap_intervals({{1, 2}, {2, 3}}) == 0);          // touching is fine
    assert(erase_overlap_intervals({}) == 0);
    assert(erase_overlap_intervals({{1, 5}}) == 0);
    // The case that defeats sorting by START: one long interval first.
    assert(erase_overlap_intervals({{1, 100}, {11, 22}, {1, 11}, {2, 12}}) == 2);

    assert(find_min_arrow_shots({{10, 16}, {2, 8}, {1, 6}, {7, 12}}) == 2);
    assert(find_min_arrow_shots({{1, 2}, {3, 4}, {5, 6}, {7, 8}}) == 4);
    assert(find_min_arrow_shots({{1, 2}, {2, 3}, {3, 4}, {4, 5}}) == 2);
    assert(find_min_arrow_shots({}) == 0);
    return 0;
}
