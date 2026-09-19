# 03 — Sliding Window

## Recognise it

Almost every question phrased as **"longest / shortest / count of contiguous
subarray-or-substring such that <condition>"**. The giveaway is *contiguous* plus an
optimization or counting goal.

## The two templates

```cpp
// (a) VARIABLE window — "longest window satisfying a condition"
std::size_t left = 0;
for (std::size_t right = 0; right < n; ++right) {
    add(v[right]);                          // extend
    while (!valid())                        // shrink until valid again
        remove(v[left++]);
    best = std::max(best, right - left + 1);
}

// (b) VARIABLE window — "shortest window satisfying a condition"
for (std::size_t right = 0; right < n; ++right) {
    add(v[right]);
    while (valid()) {                       // shrink WHILE still valid
        best = std::min(best, right - left + 1);
        remove(v[left++]);
    }
}

// (c) FIXED window of size k
for (std::size_t i = 0; i < n; ++i) {
    add(v[i]);
    if (i >= k) remove(v[i - k]);           // evict exactly one
    if (i >= k - 1) best = f(best, window_value());
}
```
The difference between (a) and (b) is **where the answer is recorded** — after restoring
validity, or while still valid. Getting that backwards is the classic bug.

## Why it is O(n)

Both pointers only move **forward**, so across the whole run each element is added once
and removed at most once. A nested `while` inside a `for` is still O(n) — be able to say
that amortized argument, because interviewers ask.

## State to maintain

Whatever `valid()` needs, updated in O(1) on add/remove:
- a **count** (sum, number of distinct, number of zeros)
- a **frequency map** (`std::array<int,128>` for ASCII, `unordered_map` otherwise)
- a **monotonic deque** for a window min/max (see section 04)

## Problems

| File | Problem | Window kind |
| --- | --- | --- |
| `01-best-time-to-buy-sell-stock.cpp` | Best Time to Buy and Sell Stock | running minimum (degenerate window) |
| `02-longest-substring-no-repeat.cpp` | Longest Substring Without Repeating Characters | longest, frequency set |
| `03-longest-repeating-char-replacement.cpp` | Longest Repeating Character Replacement | longest, count + max-frequency |
| `04-permutation-in-string.cpp` | Permutation in String | fixed size, two frequency arrays |
| `05-minimum-window-substring.cpp` | Minimum Window Substring | shortest, "have/need" counter |
| `06-sliding-window-maximum.cpp` | Sliding Window Maximum | fixed size, monotonic deque |

## Traps

1. Recording the answer in the wrong place (longest vs shortest).
2. Forgetting to update the state when shrinking.
3. Using `std::string::find`/`substr` inside the loop — that silently makes it O(n²).
4. `right - left + 1` with `std::size_t` when `right < left` can happen — it wraps.
5. For a fixed window, evicting the wrong element (`i - k`, not `i - k + 1`).
