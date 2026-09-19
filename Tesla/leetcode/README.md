# LeetCode in C++ — 17 pattern folders

**Note on the source:** the link you sent for this section was the same playlist URL as
the lecture series (a copy/paste slip — there is no LeetCode playlist at that link). So
this is built on the standard pattern taxonomy — the NeetCode-150 style roadmap — written
in **modern C++** rather than Python, because you will be coding in C++ in this
interview. Send the intended playlist and I will re-cut the folders to match its
sections.

## Why C++ specifically

A Tesla C++ interview will judge your *C++*, not just your algorithm:

- `std::vector` over raw arrays, `reserve()` when the size is known
- `std::unordered_map`/`unordered_set` and their complexity
- iterators and `std::ranges` algorithms instead of hand-rolled loops
- `std::priority_queue` (**max-heap by default** — `std::greater<>` for a min-heap)
- structured bindings, `auto`, `const&` parameters, `std::move` where it matters
- `std::optional` instead of magic sentinel values
- no leaks: `std::unique_ptr` for owned nodes, or index-based structures
- saying the complexity **and** the memory-access cost out loud

## Folder layout

```
NN-pattern-name/
├── README.md          ← how to recognise the pattern, the template, complexity
├── 01-problem-name.cpp
├── 02-problem-name.cpp
└── ...
```

Every `.cpp` file is self-contained with a `main()` full of asserts, and follows the same
structure:

```
// PROBLEM     the statement, in two lines
// EXAMPLE     input -> output
// APPROACH    the idea, in three or four lines
// COMPLEXITY  time and space, with the reason
// FOLLOW-UPS  what the interviewer asks next
// then the solution, then asserts in main()
```

## Build and run

```bash
# one file
g++ -std=c++20 -Wall -Wextra -g -fsanitize=address,undefined \
    01-arrays-and-hashing/01-two-sum.cpp -o /tmp/q && /tmp/q

# everything (also what CI would do)
bash build_all.sh
```

Always build these with sanitizers. An off-by-one in an interview is embarrassing; an
off-by-one you did not notice in practice is worse.

## The sections, in the order to do them

| # | Folder | Core idea | Do it because |
| --- | --- | --- | --- |
| 01 | [`01-arrays-and-hashing`](01-arrays-and-hashing/) | trade memory for time with a hash map | the most common warm-up |
| 02 | [`02-two-pointers`](02-two-pointers/) | two indices moving with an invariant | O(n) where sorting suggests O(n log n) |
| 03 | [`03-sliding-window`](03-sliding-window/) | a window with a maintained condition | every "longest/shortest substring" question |
| 04 | [`04-stack-and-monotonic-stack`](04-stack-and-monotonic-stack/) | defer work until you can resolve it | next-greater-element, parsing, histograms |
| 05 | [`05-binary-search`](05-binary-search/) | halve a monotonic search space | also "binary search on the answer" |
| 06 | [`06-linked-list`](06-linked-list/) | pointer surgery, fast/slow pointers | tests your pointer discipline directly |
| 07 | [`07-trees`](07-trees/) | recursion + traversal order | DFS/BFS, and the recursion-to-iteration conversion |
| 08 | [`08-tries`](08-tries/) | prefix tree over a fixed alphabet | prefix queries, autocomplete, word search |
| 09 | [`09-heap-and-priority-queue`](09-heap-and-priority-queue/) | keep the k best | top-k, streaming medians, schedulers |
| 10 | [`10-backtracking`](10-backtracking/) | choose / recurse / un-choose | permutations, subsets, constraint search |
| 11 | [`11-graphs`](11-graphs/) | BFS/DFS over a grid or adjacency list | connectivity, islands, topological order |
| 12 | [`12-advanced-graphs-and-shortest-path`](12-advanced-graphs-and-shortest-path/) | Dijkstra, union-find, MST | the weighted versions |
| 13 | [`13-dynamic-programming-1d`](13-dynamic-programming-1d/) | recurrence over one index | the pattern people fear most |
| 14 | [`14-dynamic-programming-2d`](14-dynamic-programming-2d/) | recurrence over two indices | grids, two sequences, knapsack |
| 15 | [`15-greedy-and-intervals`](15-greedy-and-intervals/) | sort, then take the locally best | scheduling — very relevant to this role |
| 16 | [`16-bit-manipulation-and-math`](16-bit-manipulation-and-math/) | bit tricks and number theory | embedded/systems flavoured |
| 17 | [`17-design-and-systems-questions`](17-design-and-systems-questions/) | **build a data structure** | **the most likely section for THIS job** |

**If you are short on time**: do 01, 02, 03, 05, 07, 11, 13, and then spend everything
left on **17**, because a foundations interview is far more likely to ask you to
implement an LRU cache, a ring buffer, or a rate limiter than to ask you for the
longest palindromic substring.

## How to practise (not how to read)

1. Read only the `PROBLEM` and `EXAMPLE` block. Cover the rest.
2. Say the approach out loud in three sentences **before** writing code.
3. State the complexity before coding, and check it against the file afterwards.
4. Write it by hand (no autocomplete). Compile. Fix your own errors.
5. Read the `FOLLOW-UPS` and answer them out loud.
6. Redo any problem you needed the solution for, 3 days later, cold.

Talking through the approach before coding is half the interview score. Practise the
talking.
