# 15 — Greedy and Intervals

## Recognise it

- **Intervals**: merging, overlap detection, scheduling, meeting rooms. Almost always
  "sort by start or by end, then sweep".
- A locally optimal choice that is **provably** globally optimal.
- Jump/reach problems: "can I get to the end", "minimum number of moves".

**The greedy trap**: it is only correct if you can argue it. Two standard arguments:
- **exchange argument**: given any optimal solution, you can swap in the greedy choice
  without making it worse — so a greedy solution is optimal too;
- **stays ahead**: after every step the greedy solution is at least as good as any other.

If you cannot make one of those arguments, the answer is probably DP. Say which one you
are using.

## The interval toolkit

```cpp
// Sort by START to MERGE overlapping intervals.
std::ranges::sort(intervals, {}, &Interval::start);

// Sort by END to maximize the count of NON-OVERLAPPING intervals
// (finish as early as possible -> leave the most room for the rest).
std::ranges::sort(intervals, {}, &Interval::end);

// Overlap test for half-open intervals [a, b):
bool overlaps = a.start < b.end && b.start < a.end;

// SWEEP LINE / difference array: +1 at each start, -1 at each end, prefix sum.
// The maximum prefix sum is the maximum concurrency -- e.g. the number of meeting rooms.
```

The **sweep line** is the most transferable idea here: it turns "how many are active at
any instant" into a sort plus a running counter, which is how you compute peak
concurrency, room counts, and resource usage over time.

## Problems

| File | Problem | Technique |
| --- | --- | --- |
| `01-merge-intervals.cpp` | Merge Intervals / Insert Interval | sort by start, extend or push |
| `02-non-overlapping-intervals.cpp` | Non-overlapping Intervals | sort by **end**, count keepers |
| `03-meeting-rooms.cpp` | Meeting Rooms I & II | sweep line / min-heap of end times |
| `04-jump-game.cpp` | Jump Game I & II | furthest reach; BFS-like level jumping |
| `05-gas-station.cpp` | Gas Station | a greedy with a real proof |
| `06-partition-labels.cpp` | Partition Labels | last-occurrence map + sweep |

## Traps

1. Sorting by the wrong key — by start for merging, by **end** for scheduling.
2. Closed `[a,b]` vs half-open `[a,b)` intervals: does touching count as overlapping?
   **Ask.**
3. Integer overflow in `start + duration`.
4. Empty input.
5. Assuming the greedy is right without an argument. If the choice can be undone later,
   it is DP, not greedy.
