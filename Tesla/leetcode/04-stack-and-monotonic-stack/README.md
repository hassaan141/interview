# 04 — Stack and Monotonic Stack

## Recognise it

- **Matching / nesting**: brackets, tags, nested encodings, expression evaluation.
- **"Next greater / next smaller element"** in any phrasing (temperatures, stock span,
  largest rectangle) → **monotonic stack**.
- You need to **defer** work until you have enough information to resolve it.
- Converting a recursive traversal to iteration (section 07).

## The monotonic stack template

```cpp
// "Next greater element to the right", storing INDICES.
std::vector<int> result(n, -1);
std::vector<std::size_t> stack;                   // indices, values DECREASING
for (std::size_t i = 0; i < n; ++i) {
    while (!stack.empty() && v[stack.back()] < v[i]) {
        result[stack.back()] = static_cast<int>(i);   // v[i] resolves the pending index
        stack.pop_back();
    }
    stack.push_back(i);
}
// whatever is left on the stack has no greater element to its right
```
Change `<` to `>` for next-*smaller*; iterate backwards for "to the left".

**Why it is O(n)**: each index is pushed exactly once and popped at most once, so the
inner `while` is amortized O(1). Say that out loud — the nested loop looks O(n²).

**Why store indices, not values**: you almost always need the position (to write the
result, or to compute a width).

## C++ notes

- Use `std::vector<T>` as the stack, not `std::stack<T>`: same complexity, contiguous
  storage, and you can iterate it for debugging. `std::stack` defaults to `std::deque`,
  which is an extra indirection.
- `std::stack::pop()` returns `void` — read `top()` first. That surprises people.
- For a bracket matcher, a `switch` plus a small lookup is clearer than a map.

## Problems

| File | Problem | Idea |
| --- | --- | --- |
| `01-valid-parentheses.cpp` | Valid Parentheses | classic matching stack |
| `02-min-stack.cpp` | Min Stack | carry the running minimum per entry |
| `03-evaluate-rpn.cpp` | Evaluate Reverse Polish Notation | operand stack |
| `04-daily-temperatures.cpp` | Daily Temperatures | monotonic stack, next greater |
| `05-car-fleet.cpp` | Car Fleet | sort by position, monotonic stack of arrival times |
| `06-largest-rectangle-histogram.cpp` | Largest Rectangle in Histogram | monotonic stack + widths |
| `07-generate-parentheses.cpp` | Generate Parentheses | backtracking with a stack-like invariant |

## Traps

1. Popping an empty stack — check `empty()` first, every time.
2. Leftover items on the stack at the end usually mean "unmatched" — handle it.
3. `<` vs `<=` in the monotonic comparison decides how equal values are grouped; for
   histograms it changes the answer.
4. `std::stack::pop()` returns nothing.
5. Integer division and negative operands in RPN (`-7 / 2` truncates toward zero in C++).
