# 14 — Dynamic Programming (2D)

## Recognise it

- **Two sequences** being compared or combined → `dp[i][j]` over the two prefixes
  (edit distance, LCS, regex matching, interleaving).
- A **grid** with a movement constraint → `dp[r][c]`.
- **Knapsack**: items × capacity.
- A **range** DP: `dp[i][j]` over a subarray (burst balloons, matrix chain).

## The method (same as 1D, with two indices)

1. State: `dp[i][j]` = the answer for the first `i` of A and the first `j` of B.
   Using **lengths** rather than indices makes `dp[0][*]` the natural empty base row.
2. Recurrence: what choices exist at `(i, j)`?
3. Base row and base column — they are where most bugs live.
4. Fill order: usually increasing `i` then `j`, but a range DP goes by increasing
   **length**.
5. Space: if `dp[i][j]` only reads row `i-1`, keep **two rows** — O(m·n) → O(n).
   If it only reads `dp[i-1][j-1]`, `dp[i-1][j]`, `dp[i][j-1]`, one row plus a saved
   diagonal is enough.

## The three recurrences to have memorized

```cpp
// Longest Common Subsequence
dp[i][j] = (a[i-1] == b[j-1]) ? dp[i-1][j-1] + 1
                              : std::max(dp[i-1][j], dp[i][j-1]);

// Edit distance (Levenshtein)
dp[i][j] = (a[i-1] == b[j-1]) ? dp[i-1][j-1]
                              : 1 + std::min({dp[i-1][j-1],   // replace
                                              dp[i-1][j],     // delete from a
                                              dp[i][j-1]});   // insert into a

// 0/1 knapsack (iterate capacity DOWNWARD for the 1D rolling version)
for (item : items)
    for (int c = capacity; c >= item.weight; --c)
        dp[c] = std::max(dp[c], dp[c - item.weight] + item.value);
```
That downward loop in knapsack is the classic: going upward turns 0/1 knapsack into
**unbounded** knapsack, because you reuse the item you just took. Say which one you mean.

## Complexity

O(m·n) time and space, reducible to O(min(m,n)) space for most. Range DP is O(n³).

## Problems

| File | Problem | State |
| --- | --- | --- |
| `01-unique-paths.cpp` | Unique Paths / with obstacles / min path sum | `dp[r][c]` = ways (or best) to reach (r,c) |
| `02-longest-common-subsequence.cpp` | LCS | `dp[i][j]` over two prefixes |
| `03-edit-distance.cpp` | Edit Distance | insert / delete / replace |
| `04-knapsack-partition.cpp` | Partition Equal Subset Sum | 0/1 knapsack on a boolean target |
| `05-longest-palindromic-substring.cpp` | Longest Palindromic Substring | range DP and expand-around-centre |

## Traps

1. Getting the base row/column wrong (they are almost never all zero).
2. Indexing confusion between "length i" and "index i" — `a[i-1]` in a length-indexed DP.
3. The knapsack loop direction.
4. Doing the O(1)-space optimization but forgetting that you now need the **old**
   `dp[i-1][j-1]` — save it in a temporary before overwriting.
5. Allocating an (m+1)×(n+1) table when two rows would do, on a large input.
