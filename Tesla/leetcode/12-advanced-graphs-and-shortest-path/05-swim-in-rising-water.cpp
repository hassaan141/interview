// PROBLEM     An n x n grid of elevations. At time t you can swim through any cell with
//             elevation <= t. Minimum t to get from (0,0) to (n-1,n-1)?
// APPROACH    Two clean formulations, and knowing both is the answer:
//             (a) DIJKSTRA-LIKE: the "cost" of a path is the MAXIMUM elevation on it
//                 (a minimax path), not the sum. Run Dijkstra with
//                     new_cost = max(current_cost, neighbour_elevation)
//                 instead of a sum. Everything else -- the min-heap, the lazy deletion --
//                 is unchanged. Recognising that Dijkstra works for any "cost" that is
//                 monotone non-decreasing along a path is the transferable insight.
//             (b) BINARY SEARCH ON THE ANSWER + BFS: is (n-1,n-1) reachable using only
//                 cells <= t? That predicate is monotonic in t, so binary search t over
//                 the elevation range (section 05).
// COMPLEXITY  (a) O(n^2 log n). (b) O(n^2 log(max_elevation)). Both fine; (a) is one pass.
// FOLLOW-UPS  Union-find alternative: add cells in increasing elevation and stop when
//             start and end are connected -- O(n^2 alpha(n)), the fastest of the three.
//             Why does Dijkstra still work with max() instead of +? -> the relaxation is
//             still monotonic (a path's cost never decreases as you extend it), which is
//             the only property the greedy finalization needs.
//             Same shape as "Path With Minimum Effort" (LC 1631) and "Swim"/"Trapping
//             Rain Water II" -- a family worth recognising.

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>

namespace {
constexpr std::array<std::pair<int, int>, 4> kDirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
}

// (a) Dijkstra with max() relaxation.
int swim_in_water(const std::vector<std::vector<int>>& grid) {
    if (grid.empty() || grid[0].empty()) return 0;
    const int n = static_cast<int>(grid.size());
    const int m = static_cast<int>(grid[0].size());

    std::vector<std::vector<int>> best(
        static_cast<std::size_t>(n),
        std::vector<int>(static_cast<std::size_t>(m), std::numeric_limits<int>::max()));

    using Entry = std::tuple<int, int, int>;              // {cost, row, col}
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> pq;
    best[0][0] = grid[0][0];
    pq.emplace(grid[0][0], 0, 0);

    while (!pq.empty()) {
        const auto [cost, r, c] = pq.top();
        pq.pop();
        if (r == n - 1 && c == m - 1) return cost;
        if (cost > best[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)]) continue;

        for (const auto& [dr, dc] : kDirs) {
            const int nr = r + dr, nc = c + dc;
            if (nr < 0 || nr >= n || nc < 0 || nc >= m) continue;
            const auto ur = static_cast<std::size_t>(nr), uc = static_cast<std::size_t>(nc);
            const int next = std::max(cost, grid[ur][uc]);   // MAX, not +
            if (next < best[ur][uc]) { best[ur][uc] = next; pq.emplace(next, nr, nc); }
        }
    }
    return -1;                                             // unreachable (cannot happen here)
}

// (b) Binary search on t + a reachability BFS. The predicate is monotonic in t.
int swim_in_water_binary_search(const std::vector<std::vector<int>>& grid) {
    if (grid.empty() || grid[0].empty()) return 0;
    const int n = static_cast<int>(grid.size());
    const int m = static_cast<int>(grid[0].size());

    const auto reachable = [&](int t) {
        if (grid[0][0] > t) return false;
        std::vector<std::vector<bool>> seen(
            static_cast<std::size_t>(n), std::vector<bool>(static_cast<std::size_t>(m), false));
        std::queue<std::pair<int, int>> q;
        q.emplace(0, 0);
        seen[0][0] = true;
        while (!q.empty()) {
            const auto [r, c] = q.front();
            q.pop();
            if (r == n - 1 && c == m - 1) return true;
            for (const auto& [dr, dc] : kDirs) {
                const int nr = r + dr, nc = c + dc;
                if (nr < 0 || nr >= n || nc < 0 || nc >= m) continue;
                const auto ur = static_cast<std::size_t>(nr), uc = static_cast<std::size_t>(nc);
                if (seen[ur][uc] || grid[ur][uc] > t) continue;
                seen[ur][uc] = true;
                q.emplace(nr, nc);
            }
        }
        return false;
    };

    int lo = grid[0][0], hi = 0;
    for (const auto& row : grid) hi = std::max(hi, *std::ranges::max_element(row));
    while (lo < hi) {                                      // first t where reachable(t)
        const int mid = lo + (hi - lo) / 2;
        if (reachable(mid)) hi = mid;
        else                lo = mid + 1;
    }
    return lo;
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn(std::vector<std::vector<int>>{{0, 2}, {1, 3}}) == 3);
        assert(fn(std::vector<std::vector<int>>{
                   {0, 1, 2, 3, 4}, {24, 23, 22, 21, 5}, {12, 13, 14, 15, 16},
                   {11, 17, 18, 19, 20}, {10, 9, 8, 7, 6}}) == 16);
        assert(fn(std::vector<std::vector<int>>{{0}}) == 0);
        assert(fn(std::vector<std::vector<int>>{{0, 1}, {2, 3}}) == 3);
        assert(fn(std::vector<std::vector<int>>{{3, 2}, {0, 1}}) == 3);   // start is highest
    };
    check([](const std::vector<std::vector<int>>& g) { return swim_in_water(g); });
    check([](const std::vector<std::vector<int>>& g) { return swim_in_water_binary_search(g); });
    assert(swim_in_water({}) == 0);
    return 0;
}
