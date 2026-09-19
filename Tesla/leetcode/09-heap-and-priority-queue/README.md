# 09 — Heap and Priority Queue

## Recognise it

- **"k largest / k smallest / kth"** anything.
- A **stream** where you must maintain a running best/median without storing everything.
- **Scheduling**: always take the next task by priority/deadline — directly relevant to
  this job.
- Merging **k** sorted sequences.
- Dijkstra and Prim (section 12).

## `std::priority_queue` in one box

```cpp
#include <queue>
std::priority_queue<int> max_heap;                                   // MAX-heap by DEFAULT
std::priority_queue<int, std::vector<int>, std::greater<>> min_heap;  // min-heap

pq.push(x); pq.emplace(a, b);   pq.top();   pq.pop();   // pop() returns VOID
// custom comparator: returns true if `a` has LOWER priority than `b`
auto by_deadline = [](const Task& a, const Task& b) { return a.deadline > b.deadline; };
std::priority_queue<Task, std::vector<Task>, decltype(by_deadline)> q{by_deadline};
```
The comparator being "inverted" relative to intuition is the #1 mistake: `std::less`
(the default) gives a **max**-heap, because the "greatest" element is at the top.

Underneath it is a binary heap over a `std::vector` — contiguous, no per-node allocation,
implicit children at `2i+1`/`2i+2`. The raw algorithms are also directly available and
are worth knowing: `std::make_heap`, `push_heap`, `pop_heap`, `sort_heap`, `is_heap`.

## Complexity

| Operation | Cost |
| --- | --- |
| push / pop | O(log n) |
| top | O(1) |
| build from n elements (`make_heap`) | **O(n)**, not O(n log n) |
| find/erase an arbitrary element | **O(n)** — a heap is not searchable |
| decrease-key | not supported; push a duplicate and skip stale entries |

## The three canonical patterns

1. **Top-k with a bounded heap of size k**: push, and pop when `size() > k`. Use a
   **min**-heap for the k *largest*. O(n log k) time, **O(k) space** — the right answer
   for a stream.
2. **`std::nth_element`** when you have the whole array in memory: **O(n)** average, beats
   the heap. Say this — reaching for a heap when `nth_element` applies is a red flag.
3. **Two heaps** for a running median: a max-heap of the lower half and a min-heap of the
   upper half, rebalanced so their sizes differ by at most one.

## Problems

| File | Problem | Pattern |
| --- | --- | --- |
| `01-kth-largest-element.cpp` | Kth Largest Element | bounded heap **vs. `nth_element`** |
| `02-k-closest-points.cpp` | K Closest Points to Origin | bounded max-heap, avoid `sqrt` |
| `03-task-scheduler.cpp` | Task Scheduler | greedy + heap; a real scheduling model |
| `04-find-median-from-stream.cpp` | Find Median from Data Stream | two heaps |
| `05-merge-k-sorted-lists.cpp` | Merge k Sorted Lists | heap over the k heads |

## Traps

1. `std::priority_queue` is a **max**-heap by default.
2. `pop()` returns `void` — read `top()` first.
3. The comparator means "lower priority", i.e. it is inverted from a sort comparator.
4. For the k **largest** you need a **min**-heap (so you can evict the smallest).
5. A heap cannot be searched or updated in place — use lazy deletion (push a new entry,
   skip stale ones on pop) or an indexed heap.
6. Comparing distances? **Do not call `sqrt`** — compare squared distances.
