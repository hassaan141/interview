# 10 — Backtracking

## Recognise it

"Generate **all** ...", "find **every** combination/permutation/partition", "is there
**any** assignment satisfying ...". The search space is exponential and you explore it as
a tree, pruning branches that cannot lead to a solution.

## The template

```cpp
void backtrack(State& state, Result& out) {
    if (is_complete(state)) { out.push_back(state); return; }   // record a leaf
    for (Choice c : choices(state)) {
        if (!is_valid(c, state)) continue;   // PRUNE -- this is where the speed comes from
        apply(c, state);                     // choose
        backtrack(state, out);               // recurse
        undo(c, state);                      // UN-choose (the "backtracking")
    }
}
```

Three things make a good backtracking answer:
1. **One mutable state buffer** plus choose/un-choose, not a fresh copy per node. Copying
   the partial solution at every node turns O(leaves) allocations into O(nodes).
2. **Pruning** as early as possible — that is the entire difference between "works for
   n=8" and "works for n=20".
3. Copying the state **only at a leaf**, where it is actually a result.

## Duplicate handling (the thing people get wrong)

When the input has duplicates and the output must be unique: **sort first**, then at each
level skip a choice equal to the previous one *unless* the previous one is in use on this
path:
```cpp
std::ranges::sort(nums);
for (std::size_t i = 0; i < nums.size(); ++i) {
    if (used[i]) continue;
    if (i > 0 && nums[i] == nums[i-1] && !used[i-1]) continue;   // the dedup rule
    ...
}
```

## Complexity

| Problem | Count | Time |
| --- | --- | --- |
| subsets | 2ⁿ | O(n·2ⁿ) |
| permutations | n! | O(n·n!) |
| combinations C(n,k) | C(n,k) | O(k·C(n,k)) |
| N-queens | — | O(n!) with pruning |

Recursion depth is O(n), so the extra space is O(n) beyond the output.

## Problems

| File | Problem | Idea |
| --- | --- | --- |
| `01-subsets.cpp` | Subsets, Subsets II | include/exclude; dedup with sorting |
| `02-combination-sum.cpp` | Combination Sum, Combination Sum II | reuse vs. no reuse; prune by sum |
| `03-permutations.cpp` | Permutations, Permutations II | swap-in-place vs. used[] |
| `04-word-search.cpp` | Word Search | grid DFS with mark/unmark |
| `05-n-queens.cpp` | N-Queens | constraint sets, the pruning showcase |
| `06-palindrome-partitioning.cpp` | Palindrome Partitioning | cut points + a DP validity table |

## Traps

1. Forgetting to **undo** the choice — the single most common bug.
2. Copying the partial state at every node instead of only at leaves.
3. Duplicate results because the input had duplicates and you did not sort + skip.
4. Passing the index wrong: `i` (reuse allowed) vs `i + 1` (each element once).
5. Not pruning, and then blaming the language for the timeout.
