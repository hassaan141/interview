// PROBLEM     Count connected regions of '1' in a grid of '1' (land) and '0' (water),
//             4-directionally connected.
// APPROACH    Scan every cell. On unvisited land, flood-fill the whole island and count
//             one. Each cell is visited once overall, so the total is O(rows*cols)
//             regardless of the island shapes.
//             Marking visited by overwriting the grid avoids an extra visited array -- a
//             real memory win on a large grid, but it MUTATES the input, so the function
//             takes the grid by value and says so.
// COMPLEXITY  Time O(rows*cols), Space O(rows*cols) worst case for the queue/stack.
// FOLLOW-UPS  DFS vs BFS: identical complexity, but recursive DFS on a 1000x1000 all-land
//             grid recurses 10^6 deep and OVERFLOWS THE STACK. BFS (or an explicit stack)
//             has the same worst-case memory but on the heap. For a systems interview,
//             volunteer this -- it is the difference between a LeetCode answer and a
//             shippable one.
//             Max island AREA (LC 695)? -> return the fill size instead of counting.
//             Islands in a STREAM of added cells (LC 305)? -> union-find with path
//             compression, near O(1) amortized per addition.
//             8-directional? -> extend the neighbour list.

#include <array>
#include <cassert>
#include <queue>
#include <utility>
#include <vector>

namespace {
constexpr std::array<std::pair<int, int>, 4> kDirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
}

// BFS flood fill: no recursion, so no stack-overflow risk on a huge grid.
int num_islands(std::vector<std::vector<char>> grid) {
    if (grid.empty() || grid[0].empty()) return 0;
    const int rows = static_cast<int>(grid.size());
    const int cols = static_cast<int>(grid[0].size());
    int islands = 0;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] != '1') continue;
            ++islands;
            std::queue<std::pair<int, int>> q;
            q.emplace(r, c);
            grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = '0';  // mark on ENQUEUE
            while (!q.empty()) {
                const auto [cr, cc] = q.front();
                q.pop();
                for (const auto& [dr, dc] : kDirs) {
                    const int nr = cr + dr, nc = cc + dc;
                    if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                    auto& cell = grid[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)];
                    if (cell != '1') continue;
                    cell = '0';                       // mark BEFORE pushing, not after popping
                    q.emplace(nr, nc);
                }
            }
        }
    }
    return islands;
}

// Largest island area: the same fill, returning a size.
int max_area_of_island(std::vector<std::vector<int>> grid) {
    if (grid.empty() || grid[0].empty()) return 0;
    const int rows = static_cast<int>(grid.size());
    const int cols = static_cast<int>(grid[0].size());
    int best = 0;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) {
            if (grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] != 1) continue;
            int area = 0;
            std::vector<std::pair<int, int>> stack{{r, c}};
            grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = 0;
            while (!stack.empty()) {
                const auto [cr, cc] = stack.back();
                stack.pop_back();
                ++area;
                for (const auto& [dr, dc] : kDirs) {
                    const int nr = cr + dr, nc = cc + dc;
                    if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                    auto& cell = grid[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)];
                    if (cell != 1) continue;
                    cell = 0;
                    stack.emplace_back(nr, nc);
                }
            }
            if (area > best) best = area;
        }
    return best;
}

int main() {
    assert(num_islands({{'1', '1', '1', '1', '0'},
                        {'1', '1', '0', '1', '0'},
                        {'1', '1', '0', '0', '0'},
                        {'0', '0', '0', '0', '0'}}) == 1);
    assert(num_islands({{'1', '1', '0', '0', '0'},
                        {'1', '1', '0', '0', '0'},
                        {'0', '0', '1', '0', '0'},
                        {'0', '0', '0', '1', '1'}}) == 3);
    assert(num_islands({}) == 0);
    assert(num_islands({{'0'}}) == 0);
    assert(num_islands({{'1'}}) == 1);
    assert(num_islands({{'1', '0', '1', '0', '1'}}) == 3);      // single row
    assert(num_islands({{'1'}, {'0'}, {'1'}}) == 2);            // single column

    assert(max_area_of_island({{1, 1, 0}, {1, 0, 0}, {0, 0, 1}}) == 3);
    assert(max_area_of_island({{0, 0}, {0, 0}}) == 0);

    // A large all-land grid: BFS handles it; a recursive DFS would blow the stack.
    std::vector<std::vector<char>> big(400, std::vector<char>(400, '1'));
    assert(num_islands(big) == 1);
    return 0;
}
