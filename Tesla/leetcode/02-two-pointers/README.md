# 02 — Two Pointers

## Recognise it

- The input is **sorted** (or you can sort it) and you need a pair/triple with a property.
- You are scanning from **both ends** toward the middle (palindromes, container with most
  water, reverse in place).
- You need to **partition or compact in place** (remove duplicates, move zeroes) — a
  read pointer and a write pointer.
- **Fast/slow** pointers on a sequence or linked list (cycle detection, middle element).

The point is an O(n) or O(n log n) answer where the brute force is O(n²), with **O(1)
extra space**.

## The three shapes

```cpp
// (a) Opposite ends, converging. Needs a sorted input or a symmetric property.
std::size_t lo = 0, hi = v.size() - 1;        // careful: empty vector underflows!
while (lo < hi) {
    if (condition_says_increase_lo) ++lo;
    else if (condition_says_decrease_hi) --hi;
    else { /* found */ ++lo; --hi; }
}

// (b) Read/write (in-place compaction). The write pointer lags the read pointer.
std::size_t write = 0;
for (std::size_t read = 0; read < v.size(); ++read)
    if (keep(v[read])) v[write++] = std::move(v[read]);
v.resize(write);                               // or return write as the new length
// ...which is exactly what std::remove_if + erase does (course section 12).

// (c) Fast/slow. slow advances 1, fast advances 2.
auto slow = head, fast = head;
while (fast && fast->next) { slow = slow->next; fast = fast->next->next; }
// slow is now the middle; if fast ever meets slow, there is a cycle (Floyd).
```

## Why the invariant matters

Every two-pointer proof is: *"moving this pointer can only make the answer worse, so
nothing is skipped."* In Container With Most Water: moving the **taller** wall inward can
never increase the area (width shrinks and the height is capped by the shorter wall), so
moving the shorter one is safe. **Be able to say the invariant out loud** — that is what
separates "I memorized it" from "I can derive it".

## Complexity

O(n) time, O(1) extra space, after an O(n log n) sort if one is needed. Each pointer moves
monotonically, so the total work is bounded by n.

## Problems

| File | Problem | Key idea |
| --- | --- | --- |
| `01-valid-palindrome.cpp` | Valid Palindrome | converge from both ends, skip non-alphanumerics |
| `02-two-sum-sorted.cpp` | Two Sum II (sorted input) | O(1) space version of Two Sum |
| `03-three-sum.cpp` | 3Sum | sort, fix one, two-pointer the rest, skip duplicates |
| `04-container-with-most-water.cpp` | Container With Most Water | move the shorter wall; prove it |
| `05-trapping-rain-water.cpp` | Trapping Rain Water | two pointers + running max from each side |
| `06-remove-duplicates-sorted.cpp` | Remove Duplicates from Sorted Array | read/write compaction |
| `07-sort-colors.cpp` | Sort Colors (Dutch national flag) | three-way partition in one pass |

## Traps

1. `v.size() - 1` on an **empty** vector wraps to `SIZE_MAX` (course section 01).
2. `while (lo < hi)` vs `<=` — decide whether the middle element must be examined.
3. Duplicate skipping in 3Sum must happen for **all three** positions, or you emit
   duplicate triples.
4. Overflow in `a + b` when summing two large `int`s — use `long long` or check.
5. In-place compaction must `std::move` (not copy) non-trivial elements.
