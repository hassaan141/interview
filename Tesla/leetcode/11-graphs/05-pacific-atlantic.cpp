// PROBLEM     Water flows from a cell to a neighbour of equal or LOWER height. The Pacific
//             touches the top and left edges, the Atlantic the bottom and right. Return
//             every cell from which water can reach BOTH oceans.
// APPROACH    REVERSE THE FLOW. A forward search from each cell is O((rows*cols)^2).
//             Instead start at the ocean borders and walk UPHILL (to neighbours of equal
//             or greater height), marking everything reachable. Do that once per ocean and
//             intersect the two reachable sets.
//             "Run the search backwards from the targets" is the transferable idea -- it
//             shows up whenever many sources share few sinks.
// COMPLEXITY  Time O(rows*cols) -- two traversals, each visiting every cell at most once.
//             Space O(rows*cols) for the two boolean grids.
// FOLLOW-UPS  Why is the forward version quadratic? -> a separate search per start cell.
//             Multi-source again: both border walks are multi-source BFS/DFS (problem 03).
//             Equal heights? -> allowed in both directions, so the >= comparison matters;
//             a plain > silently drops plateaus.

#include <array>
#include <cassert>
#include <queue>
#include <utility>
#include <vector>

namespace {
constexpr std::array<std::pair<int, int>, 4> kDirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

// Multi-source BFS walking UPHILL from the given border cells.
void flow_uphill(const std::vector<std::vector<int>>& h,
                 std::queue<std::pair<int, int>>& q, std::vector<std::vector<bool>>& seen) {
    const int rows = static_cast<int>(h.size());
    const int cols = static_cast<int>(h[0].size());
    while (!q.empty()) {
        const auto [r, c] = q.front();
        q.pop();
        for (const auto& [dr, dc] : kDirs) {
            const int nr = r + dr, nc = c + dc;
            if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
            const auto ur = static_cast<std::size_t>(nr), uc = static_cast<std::size_t>(nc);
            if (seen[ur][uc]) continue;
            // Reversed: we may move to a neighbour that is at least as HIGH, because
            // water would flow from there down to us.
            if (h[ur][uc] < h[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)]) continue;
            seen[ur][uc] = true;
            q.emplace(nr, nc);
        }
    }
}
}  // namespace

std::vector<std::pair<int, int>> pacific_atlantic(const std::vector<std::vector<int>>& h) {
    std::vector<std::pair<int, int>> out;
    if (h.empty() || h[0].empty()) return out;
    const int rows = static_cast<int>(h.size());
    const int cols = static_cast<int>(h[0].size());

    std::vector<std::vector<bool>> pac(static_cast<std::size_t>(rows),
                                       std::vector<bool>(static_cast<std::size_t>(cols), false));
    std::vector<std::vector<bool>> atl = pac;
    std::queue<std::pair<int, int>> qp, qa;

    for (int r = 0; r < rows; ++r) {
        pac[static_cast<std::size_t>(r)][0] = true;                 qp.emplace(r, 0);
        atl[static_cast<std::size_t>(r)][static_cast<std::size_t>(cols - 1)] = true;
        qa.emplace(r, cols - 1);
    }
    for (int c = 0; c < cols; ++c) {
        pac[0][static_cast<std::size_t>(c)] = true;                 qp.emplace(0, c);
        atl[static_cast<std::size_t>(rows - 1)][static_cast<std::size_t>(c)] = true;
        qa.emplace(rows - 1, c);
    }

    flow_uphill(h, qp, pac);
    flow_uphill(h, qa, atl);

    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            if (pac[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] &&
                atl[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)])
                out.emplace_back(r, c);
    return out;
}

int main() {
    const std::vector<std::vector<int>> heights{
        {1, 2, 2, 3, 5},
        {3, 2, 3, 4, 4},
        {2, 4, 5, 3, 1},
        {6, 7, 1, 4, 5},
        {5, 1, 1, 2, 4}};
    const auto cells = pacific_atlantic(heights);
    assert(cells.size() == 7);

    assert(pacific_atlantic({}).empty());
    assert(pacific_atlantic({{}}).empty());
    assert(pacific_atlantic({{1}}).size() == 1);                 // one cell touches both

    // A flat grid: every cell reaches both oceans.
    const std::vector<std::vector<int>> flat(3, std::vector<int>(3, 5));
    assert(pacific_atlantic(flat).size() == 9);

    // A single row: every cell borders both the top (Pacific) and bottom (Atlantic).
    assert(pacific_atlantic({{1, 2, 3}}).size() == 3);
    return 0;
}
