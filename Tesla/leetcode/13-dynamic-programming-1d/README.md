# 13 — Dynamic Programming (1D)

## The method (use it every time, in this order)

1. **Define the state.** "`dp[i]` = the answer for the prefix ending at / of length i."
   Write it down as an English sentence. Most DP failures are a bad state definition, not
   a bad recurrence.
2. **Write the recurrence.** How does `dp[i]` follow from smaller states? What choices do
   you have at step i?
3. **Base cases.** `dp[0]` (and sometimes `dp[1]`) — and be honest about what "empty"
   means.
4. **Order of evaluation.** Bottom-up usually means increasing i.
5. **Optimize the space.** If `dp[i]` only reads `dp[i-1]` and `dp[i-2]`, you need two
   variables, not an array. That O(n) → O(1) step is what interviewers look for.

## Top-down vs. bottom-up

```cpp
// Top-down (memoized recursion): closest to the recurrence, easiest to derive.
int solve(int i, std::vector<int>& memo) {
    if (i <= 1) return base;
    int& m = memo[i];
    if (m != kUnset) return m;
    return m = f(solve(i-1, memo), solve(i-2, memo));
}
// Bottom-up: no recursion (no stack overflow), better cache behaviour, and it is the
// only form that easily supports the O(1)-space rolling optimization.
```
Derive top-down, ship bottom-up. Say that.

## Recognising 1D DP

- "In how many ways can I...", "what is the maximum/minimum ... over a sequence".
- A choice at each step with **overlapping subproblems** (the same subproblem recurs) and
  **optimal substructure** (the best overall uses the best of a subproblem).
- Greedy fails because a locally best choice can be globally wrong — that is the signal.

## Problems

| File | Problem | State |
| --- | --- | --- |
| `01-climbing-stairs.cpp` | Climbing Stairs / Min Cost | `dp[i]` = ways to reach step i |
| `02-house-robber.cpp` | House Robber I & II | `dp[i]` = best loot from the first i houses |
| `03-coin-change.cpp` | Coin Change | `dp[a]` = fewest coins making amount a |
| `04-longest-increasing-subsequence.cpp` | LIS | `dp[i]` = LIS ending at i; plus the O(n log n) patience version |
| `05-word-break.cpp` | Word Break | `dp[i]` = is the prefix of length i splittable |
| `06-max-subarray.cpp` | Maximum Subarray (Kadane) | `dp[i]` = best subarray **ending at** i |
| `07-decode-ways.cpp` | Decode Ways | `dp[i]` = decodings of the first i characters |

## Traps

1. A state that does not carry enough information (the "ending at i" vs. "over the first
   i" distinction is the most common one).
2. Off-by-one between "length i" and "index i" — pick one convention and write it in a
   comment.
3. Missing base cases, especially the empty case.
4. Using an `int` sentinel that overflows when you add to it (`INT_MAX + 1`).
5. Not doing the O(1)-space rolling optimization when the recurrence allows it.
6. Reaching for DP when greedy is provably correct, or vice versa.
