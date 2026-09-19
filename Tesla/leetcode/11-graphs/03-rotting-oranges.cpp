// PROBLEM     Grid of 0 (empty), 1 (fresh orange), 2 (rotten). Each minute, every rotten
//             orange rots its 4-neighbours. Minutes until none are fresh, or -1.
// APPROACH    MULTI-SOURCE BFS: seed the queue with EVERY initially rotten cell, then
//             expand level by level -- each level is one minute. Multi-source BFS is the
//             pattern to recognise; it computes the distance to the NEAREST source for
//             every cell in one O(V+E) pass, instead of running a BFS per source.
//             Count the fresh oranges up front so the -1 case (unreachable oranges) is a
//             simple final comparison rather than a rescan.
// COMPLEXITY  Time O(rows*cols), Space O(rows*cols).
// FOLLOW-UPS  Why not a BFS per rotten orange? -> that is O(sources * V), and it computes
//             the wrong thing anyway (you want the minimum over sources, which one
//             multi-source pass gives directly).
//             This is the same pattern as "01 Matrix" (LC 542, distance to the nearest 0)
//             and "Walls and Gates" (LC 286). Recognising the family is the point.
//             Diagonal spread, or different rates per cell? -> the latter is no longer
//             BFS; it becomes Dijkstra (section 12).

#include <array>
#include <cassert>
#include <queue>
#include <utility>
#include <vector>

int oranges_rotting(std::vector<std::vector<int>> grid) {
    if (grid.empty() || grid[0].empty()) return 0;
    const int rows = static_cast<int>(grid.size());
    const int cols = static_cast<int>(grid[0].size());
    constexpr std::array<std::pair<int, int>, 4> kDirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

    std::queue<std::pair<int, int>> q;
    int fresh = 0;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) {
            const int v = grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            if (v == 2) q.emplace(r, c);                 // EVERY source seeds the queue
            else if (v == 1) ++fresh;
        }
    if (fresh == 0) return 0;                            // nothing to rot: 0 minutes

    int minutes = 0;
    while (!q.empty() && fresh > 0) {
        const std::size_t level = q.size();              // one level == one minute
        for (std::size_t i = 0; i < level; ++i) {
            const auto [r, c] = q.front();
            q.pop();
            for (const auto& [dr, dc] : kDirs) {
                const int nr = r + dr, nc = c + dc;
                if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                auto& cell = grid[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)];
                if (cell != 1) continue;
                cell = 2;                                 // mark on ENQUEUE
                --fresh;
                q.emplace(nr, nc);
            }
        }
        ++minutes;
    }
    return fresh == 0 ? minutes : -1;                     // unreachable oranges remain
}

int main() {
    assert(oranges_rotting({{2, 1, 1}, {1, 1, 0}, {0, 1, 1}}) == 4);
    assert(oranges_rotting({{2, 1, 1}, {0, 1, 1}, {1, 0, 1}}) == -1);   // isolated orange
    assert(oranges_rotting({{0, 2}}) == 0);                              // nothing fresh
    assert(oranges_rotting({{0}}) == 0);
    assert(oranges_rotting({{1}}) == -1);                                // no source
    assert(oranges_rotting({{2}}) == 0);
    assert(oranges_rotting({}) == 0);
    assert(oranges_rotting({{2, 1, 1, 1, 1}}) == 4);                     // a line
    // Multi-source: two sources meet in the middle, so it is faster than one source.
    assert(oranges_rotting({{2, 1, 1, 1, 2}}) == 2);
    return 0;
}
