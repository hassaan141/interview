// PROBLEM     (a) Merge all overlapping intervals. (b) Insert a new interval into a
//             sorted, non-overlapping list and merge as needed.
// EXAMPLE     [[1,3],[2,6],[8,10],[15,18]] -> [[1,6],[8,10],[15,18]]
// APPROACH    SORT BY START. Then sweep: if the next interval starts at or before the
//             current one's end, extend the end to the max of the two; otherwise the
//             current interval is finished, so emit it and start a new one.
//             Sorting by start is what makes a single pass sufficient -- after sorting,
//             anything that overlaps the current interval must start within it.
// COMPLEXITY  Time O(n log n) dominated by the sort, O(n) after. Space O(n) for the output
//             (O(1) extra if you merge in place).
// FOLLOW-UPS  Closed [a,b] or half-open [a,b)? -> it decides whether [1,2] and [2,3]
//             merge. ASK; do not assume. This code treats them as CLOSED (touching
//             merges), which is the LeetCode convention.
//             (b) Insert into an already sorted list? -> O(n), no sort needed: copy the
//             intervals entirely before, merge the overlapping run, copy the rest.
//             Streaming intervals? -> keep them in a std::map keyed by start and merge with
//             the neighbours on insert, O(log n) each.
//             Overflow? -> start + duration can exceed int; use long long for the
//             arithmetic if durations are large.

#include <algorithm>
#include <array>
#include <cassert>
#include <vector>

using Interval = std::array<int, 2>;                  // {start, end}, CLOSED

std::vector<Interval> merge_intervals(std::vector<Interval> intervals) {
    if (intervals.empty()) return {};
    std::ranges::sort(intervals);                      // by start, then end

    std::vector<Interval> out;
    out.reserve(intervals.size());
    out.push_back(intervals.front());
    for (std::size_t i = 1; i < intervals.size(); ++i) {
        Interval& last = out.back();
        if (intervals[i][0] <= last[1])                 // <= : touching counts as overlap
            last[1] = std::max(last[1], intervals[i][1]);
        else
            out.push_back(intervals[i]);
    }
    return out;
}

// (b) The input is already sorted and non-overlapping: O(n), no sort.
std::vector<Interval> insert_interval(const std::vector<Interval>& intervals,
                                      Interval fresh) {
    std::vector<Interval> out;
    out.reserve(intervals.size() + 1);
    std::size_t i = 0;
    const std::size_t n = intervals.size();

    while (i < n && intervals[i][1] < fresh[0]) out.push_back(intervals[i++]);   // before
    while (i < n && intervals[i][0] <= fresh[1]) {                                // overlap
        fresh[0] = std::min(fresh[0], intervals[i][0]);
        fresh[1] = std::max(fresh[1], intervals[i][1]);
        ++i;
    }
    out.push_back(fresh);
    while (i < n) out.push_back(intervals[i++]);                                  // after
    return out;
}

int main() {
    assert((merge_intervals({{1, 3}, {2, 6}, {8, 10}, {15, 18}})
            == std::vector<Interval>{{1, 6}, {8, 10}, {15, 18}}));
    assert((merge_intervals({{1, 4}, {4, 5}}) == std::vector<Interval>{{1, 5}}));  // touching
    assert(merge_intervals({}).empty());
    assert((merge_intervals({{1, 4}}) == std::vector<Interval>{{1, 4}}));
    assert((merge_intervals({{1, 4}, {2, 3}}) == std::vector<Interval>{{1, 4}}));  // contained
    assert((merge_intervals({{5, 6}, {1, 2}}) == std::vector<Interval>{{1, 2}, {5, 6}}));
    assert((merge_intervals({{1, 1}, {1, 1}}) == std::vector<Interval>{{1, 1}}));  // points

    assert((insert_interval({{1, 3}, {6, 9}}, {2, 5})
            == std::vector<Interval>{{1, 5}, {6, 9}}));
    assert((insert_interval({{1, 2}, {3, 5}, {6, 7}, {8, 10}, {12, 16}}, {4, 8})
            == std::vector<Interval>{{1, 2}, {3, 10}, {12, 16}}));
    assert((insert_interval({}, {5, 7}) == std::vector<Interval>{{5, 7}}));
    assert((insert_interval({{1, 5}}, {2, 3}) == std::vector<Interval>{{1, 5}}));
    assert((insert_interval({{1, 5}}, {6, 8}) == std::vector<Interval>{{1, 5}, {6, 8}}));
    return 0;
}
