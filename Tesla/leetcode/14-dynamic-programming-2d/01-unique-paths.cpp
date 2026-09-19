// PROBLEM     (a) Number of paths from the top-left to the bottom-right of an m x n grid
//             moving only right or down. (b) With obstacles. (c) Minimum path sum.
// APPROACH    STATE: dp[r][c] = the number of ways (or best cost) to reach (r,c).
//             RECURRENCE: you arrive from above or from the left, and those path sets are
//             disjoint, so dp[r][c] = dp[r-1][c] + dp[r][c-1].
//             BASE: the first row and first column are all 1 (only one straight path).
//             SPACE: dp[r][c] reads only the row above and the cell to the left, so ONE
//             row suffices, updated in place: row[c] += row[c-1]. That is the optimization
//             to volunteer -- O(m*n) -> O(n).
// COMPLEXITY  Time O(m*n), Space O(n).
// FOLLOW-UPS  Closed form for (a)? -> C(m+n-2, m-1): you make m-1 downs and n-1 rights in
//             some order. O(min(m,n)) with a careful multiply, and be ready to discuss
//             overflow (use long long and divide as you go, never compute the factorials).
//             Obstacles (b)? -> a blocked cell is 0 ways, including in the base row, where
//             everything after an obstacle must also be 0.
//             Minimum path sum (c)? -> the same skeleton with min() and += cost.
//             Diagonal moves allowed? -> add dp[r-1][c-1].

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

// (a) O(n) space rolling row.
long long unique_paths(int m, int n) {
    if (m <= 0 || n <= 0) return 0;
    std::vector<long long> row(static_cast<std::size_t>(n), 1);   // the top row: all 1
    for (int r = 1; r < m; ++r)
        for (int c = 1; c < n; ++c)
            row[static_cast<std::size_t>(c)] += row[static_cast<std::size_t>(c - 1)];
    return row[static_cast<std::size_t>(n - 1)];
}

// (b) With obstacles (1 = blocked).
long long unique_paths_with_obstacles(const std::vector<std::vector<int>>& grid) {
    if (grid.empty() || grid[0].empty()) return 0;
    const std::size_t n = grid[0].size();
    std::vector<long long> row(n, 0);
    row[0] = grid[0][0] ? 0 : 1;
    for (std::size_t r = 0; r < grid.size(); ++r)
        for (std::size_t c = 0; c < n; ++c) {
            if (grid[r][c] == 1) { row[c] = 0; continue; }         // blocked: 0 ways
            if (c > 0) row[c] += row[c - 1];                        // else carry from above
        }
    return row[n - 1];
}

// (c) Minimum path sum: the same shape with min().
int min_path_sum(const std::vector<std::vector<int>>& grid) {
    if (grid.empty() || grid[0].empty()) return 0;
    const std::size_t n = grid[0].size();
    constexpr int kInf = std::numeric_limits<int>::max() / 4;
    std::vector<int> row(n, kInf);
    row[0] = 0;
    for (std::size_t r = 0; r < grid.size(); ++r)
        for (std::size_t c = 0; c < n; ++c) {
            if (c > 0) row[c] = std::min(row[c], row[c - 1]);       // from above or left
            row[c] += grid[r][c];
        }
    return row[n - 1];
}

int main() {
    assert(unique_paths(3, 7) == 28);
    assert(unique_paths(3, 2) == 3);
    assert(unique_paths(1, 1) == 1);
    assert(unique_paths(1, 10) == 1);                 // a single row
    assert(unique_paths(0, 5) == 0);
    assert(unique_paths(10, 10) == 48620);

    assert(unique_paths_with_obstacles({{0, 0, 0}, {0, 1, 0}, {0, 0, 0}}) == 2);
    assert(unique_paths_with_obstacles({{0, 1}, {0, 0}}) == 1);
    assert(unique_paths_with_obstacles({{1}}) == 0);              // start blocked
    assert(unique_paths_with_obstacles({{0, 0}, {0, 1}}) == 0);   // end blocked
    assert(unique_paths_with_obstacles({{0}}) == 1);
    assert(unique_paths_with_obstacles({{0, 1, 0}}) == 0);        // blocked base row

    assert(min_path_sum({{1, 3, 1}, {1, 5, 1}, {4, 2, 1}}) == 7);
    assert(min_path_sum({{1, 2, 3}, {4, 5, 6}}) == 12);
    assert(min_path_sum({{5}}) == 5);
    assert(min_path_sum({}) == 0);
    return 0;
}
