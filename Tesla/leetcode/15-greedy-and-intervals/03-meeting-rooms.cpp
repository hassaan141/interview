// PROBLEM     (a) Can one person attend all meetings (no overlaps)?
//             (b) Minimum number of rooms needed for all of them?
// APPROACH    (a) Sort by start and check adjacent pairs. O(n log n).
//             (b) Two equivalent optimal answers, and knowing both is the point:
//               - SWEEP LINE: turn each meeting into (+1 at start, -1 at end), sort all
//                 the events by time, and take the running sum's maximum. Process ENDS
//                 before STARTS at the same timestamp, or a room that frees at exactly
//                 the moment another begins is double-counted.
//               - MIN-HEAP of end times: for each meeting in start order, pop every room
//                 whose meeting has ended, then push this one. The heap size is the room
//                 count; its maximum is the answer.
//             The sweep line is the more general tool: it answers "peak concurrency" for
//             any resource, which is exactly how you size a thread pool, a buffer pool, or
//             a connection pool from a trace.
// COMPLEXITY  Both O(n log n) time, O(n) space.
// FOLLOW-UPS  Which meetings are in which room? -> the heap version can carry a room id.
//             Streaming/online? -> the heap version works incrementally; the sweep needs
//             all the events.
//             Half-open vs closed intervals decides the tie rule -- ASK.
//             The sweep-line generalization is a DIFFERENCE ARRAY: +1/-1 then a prefix
//             sum, which is O(n + range) when the times are small integers.

#include <algorithm>
#include <array>
#include <cassert>
#include <queue>
#include <vector>

using Interval = std::array<int, 2>;                  // {start, end}, half-open

bool can_attend_all(std::vector<Interval> meetings) {
    std::ranges::sort(meetings);
    for (std::size_t i = 1; i < meetings.size(); ++i)
        if (meetings[i][0] < meetings[i - 1][1]) return false;   // < : touching is OK
    return true;
}

// (b) Sweep line: the general "peak concurrency" tool.
int min_meeting_rooms_sweep(const std::vector<Interval>& meetings) {
    std::vector<std::pair<int, int>> events;           // {time, delta}
    events.reserve(meetings.size() * 2);
    for (const Interval& m : meetings) {
        events.emplace_back(m[0], +1);
        events.emplace_back(m[1], -1);
    }
    // Sorting by {time, delta} puts -1 (an end) before +1 (a start) at the same time,
    // so a room freed at time t is reused at time t. That tie-break IS the bug people hit.
    std::ranges::sort(events);

    int active = 0, peak = 0;
    for (const auto& [time, delta] : events) {
        active += delta;
        peak = std::max(peak, active);
    }
    return peak;
}

// (b) Min-heap of end times: works incrementally on a stream.
int min_meeting_rooms_heap(std::vector<Interval> meetings) {
    if (meetings.empty()) return 0;
    std::ranges::sort(meetings);                        // by start
    std::priority_queue<int, std::vector<int>, std::greater<>> ends;   // MIN-heap
    for (const Interval& m : meetings) {
        if (!ends.empty() && ends.top() <= m[0]) ends.pop();   // a room freed up
        ends.push(m[1]);
    }
    return static_cast<int>(ends.size());
}

int main() {
    assert(!can_attend_all({{0, 30}, {5, 10}, {15, 20}}));
    assert(can_attend_all({{7, 10}, {2, 4}}));
    assert(can_attend_all({}));
    assert(can_attend_all({{1, 5}}));
    assert(can_attend_all({{1, 5}, {5, 10}}));          // touching, half-open: fine

    const auto check = [](auto&& fn) {
        assert(fn(std::vector<Interval>{{0, 30}, {5, 10}, {15, 20}}) == 2);
        assert(fn(std::vector<Interval>{{7, 10}, {2, 4}}) == 1);
        assert(fn(std::vector<Interval>{}) == 0);
        assert(fn(std::vector<Interval>{{1, 5}}) == 1);
        assert(fn(std::vector<Interval>{{1, 5}, {5, 10}}) == 1);      // reuse the room
        assert(fn(std::vector<Interval>{{1, 10}, {2, 7}, {3, 19}, {8, 12}, {10, 20}, {11, 30}}) == 4);
        assert(fn(std::vector<Interval>{{1, 2}, {1, 2}, {1, 2}}) == 3);   // all concurrent
    };
    check([](const std::vector<Interval>& m) { return min_meeting_rooms_sweep(m); });
    check([](std::vector<Interval> m) { return min_meeting_rooms_heap(std::move(m)); });
    return 0;
}
