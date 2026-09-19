// PROBLEM     Largest rectangle that fits entirely under a histogram.
// EXAMPLE     [2,1,5,6,2,3] -> 10  (heights 5 and 6, width 2)
// APPROACH    For each bar, the largest rectangle with THAT bar's height extends left and
//             right until a shorter bar. A monotonic INCREASING stack of indices finds
//             both boundaries in one pass: when bar i is shorter than the stack top, the
//             top's rectangle is resolved -- its right boundary is i, and its left
//             boundary is the new stack top (the previous shorter bar).
//             The width is therefore  i - stack.back() - 1  after popping, NOT i - popped.
//             That off-by-one is the whole difficulty of this problem.
// COMPLEXITY  Time O(n) amortized, Space O(n). Brute force is O(n^2).
// FOLLOW-UPS  Maximal Rectangle in a binary MATRIX (LC 85)? -> run this per row over a
//             running "heights" array: O(rows * cols).
//             Why push a sentinel 0 at the end? -> it forces every remaining bar to be
//             resolved, avoiding a duplicated drain loop after the main loop.

#include <algorithm>
#include <cassert>
#include <vector>

int largest_rectangle_area(std::vector<int> heights) {
    heights.push_back(0);                              // sentinel: drains the stack
    std::vector<std::size_t> stack;                     // indices, heights INCREASING
    stack.reserve(heights.size());
    int best = 0;

    for (std::size_t i = 0; i < heights.size(); ++i) {
        while (!stack.empty() && heights[stack.back()] >= heights[i]) {
            const int h = heights[stack.back()];
            stack.pop_back();
            // Left boundary is the new top (or -1 if the stack is empty), right is i.
            const std::size_t left = stack.empty() ? 0 : stack.back() + 1;
            const int width = static_cast<int>(i - left);
            best = std::max(best, h * width);
        }
        stack.push_back(i);
    }
    return best;
}

// Maximal rectangle of 1s in a binary matrix -- this problem applied per row.
int maximal_rectangle(const std::vector<std::vector<int>>& matrix) {
    if (matrix.empty() || matrix[0].empty()) return 0;
    std::vector<int> heights(matrix[0].size(), 0);
    int best = 0;
    for (const auto& row : matrix) {
        for (std::size_t c = 0; c < row.size(); ++c)
            heights[c] = row[c] ? heights[c] + 1 : 0;   // running column heights
        best = std::max(best, largest_rectangle_area(heights));
    }
    return best;
}

int main() {
    assert(largest_rectangle_area({2, 1, 5, 6, 2, 3}) == 10);
    assert(largest_rectangle_area({2, 4}) == 4);
    assert(largest_rectangle_area({}) == 0);
    assert(largest_rectangle_area({5}) == 5);
    assert(largest_rectangle_area({1, 1, 1, 1}) == 4);       // flat
    assert(largest_rectangle_area({5, 4, 3, 2, 1}) == 9);    // decreasing
    assert(largest_rectangle_area({1, 2, 3, 4, 5}) == 9);    // increasing
    assert(largest_rectangle_area({0, 0}) == 0);

    assert(maximal_rectangle({{1, 0, 1, 0, 0},
                              {1, 0, 1, 1, 1},
                              {1, 1, 1, 1, 1},
                              {1, 0, 0, 1, 0}}) == 6);
    assert(maximal_rectangle({}) == 0);
    assert(maximal_rectangle({{0}}) == 0);
    assert(maximal_rectangle({{1}}) == 1);
    return 0;
}
