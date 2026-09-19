// PROBLEM     Connect all points with minimum total Manhattan distance -- a minimum
//             spanning tree on a complete graph.
// APPROACH    Two MST algorithms, and the choice depends on DENSITY:
//               PRIM  -- grow one tree, always adding the cheapest edge leaving it.
//                        O(V^2) with an array scan, which is OPTIMAL for a complete graph
//                        like this one (E = V^2, so a heap gives O(V^2 log V) -- worse).
//               KRUSKAL -- sort all edges, add each if it joins two different components
//                        (union-find). O(E log E); better for a SPARSE graph.
//             This problem is complete, so Prim with an array scan is the right pick --
//             and saying why is the point of the question.
// COMPLEXITY  Prim O(V^2) time, O(V) space. Kruskal O(V^2 log V) here because generating
//             the V^2 edges dominates.
// FOLLOW-UPS  Why does the greedy work? -> the CUT PROPERTY: for any partition of the
//             vertices, the lightest edge crossing it is in some MST. Both algorithms are
//             instances of that.
//             Sparse graph? -> Kruskal, or Prim with a heap.
//             Real use: MST shows up in clustering, network design, and as a heuristic
//             lower bound for TSP.

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <numeric>
#include <vector>

using Point = std::array<int, 2>;

static int manhattan(const Point& a, const Point& b) {
    return std::abs(a[0] - b[0]) + std::abs(a[1] - b[1]);
}

// PRIM with an array scan: O(V^2), optimal for a complete graph.
int min_cost_connect_prim(const std::vector<Point>& points) {
    const std::size_t n = points.size();
    if (n < 2) return 0;
    std::vector<int> cheapest(n, std::numeric_limits<int>::max());
    std::vector<bool> in_tree(n, false);
    cheapest[0] = 0;
    int total = 0;

    for (std::size_t added = 0; added < n; ++added) {
        // Pick the cheapest vertex not yet in the tree.
        std::size_t best = n;
        for (std::size_t i = 0; i < n; ++i)
            if (!in_tree[i] && (best == n || cheapest[i] < cheapest[best])) best = i;

        in_tree[best] = true;
        total += cheapest[best];
        // Relax: update every outside vertex's cheapest connection to the tree.
        for (std::size_t i = 0; i < n; ++i)
            if (!in_tree[i])
                cheapest[i] = std::min(cheapest[i], manhattan(points[best], points[i]));
    }
    return total;
}

// KRUSKAL with union-find: better when the graph is sparse.
namespace {
class UnionFind {
    std::vector<int> parent_, size_;
public:
    explicit UnionFind(std::size_t n) : parent_(n), size_(n, 1) {
        std::iota(parent_.begin(), parent_.end(), 0);
    }
    int find(int x) {
        while (parent_[static_cast<std::size_t>(x)] != x) {
            parent_[static_cast<std::size_t>(x)] =
                parent_[static_cast<std::size_t>(parent_[static_cast<std::size_t>(x)])];
            x = parent_[static_cast<std::size_t>(x)];
        }
        return x;
    }
    bool unite(int a, int b) {
        int ra = find(a), rb = find(b);
        if (ra == rb) return false;
        if (size_[static_cast<std::size_t>(ra)] < size_[static_cast<std::size_t>(rb)])
            std::swap(ra, rb);
        parent_[static_cast<std::size_t>(rb)] = ra;
        size_[static_cast<std::size_t>(ra)] += size_[static_cast<std::size_t>(rb)];
        return true;
    }
};
}  // namespace

int min_cost_connect_kruskal(const std::vector<Point>& points) {
    const std::size_t n = points.size();
    if (n < 2) return 0;
    struct Edge { int weight; int a, b; };
    std::vector<Edge> edges;
    edges.reserve(n * (n - 1) / 2);
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = i + 1; j < n; ++j)
            edges.push_back({manhattan(points[i], points[j]),
                             static_cast<int>(i), static_cast<int>(j)});

    std::ranges::sort(edges, {}, &Edge::weight);
    UnionFind uf{n};
    int total = 0;
    std::size_t used = 0;
    for (const Edge& e : edges) {
        if (!uf.unite(e.a, e.b)) continue;               // would close a cycle
        total += e.weight;
        if (++used == n - 1) break;                       // an MST has exactly n-1 edges
    }
    return total;
}

int main() {
    const std::vector<Point> pts{{0, 0}, {2, 2}, {3, 10}, {5, 2}, {7, 0}};
    assert(min_cost_connect_prim(pts) == 20);
    assert(min_cost_connect_kruskal(pts) == 20);

    const auto check = [](auto&& fn) {
        assert(fn(std::vector<Point>{{3, 12}, {-2, 5}, {-4, 1}}) == 18);
        assert(fn(std::vector<Point>{}) == 0);
        assert(fn(std::vector<Point>{{0, 0}}) == 0);
        assert(fn(std::vector<Point>{{0, 0}, {1, 1}}) == 2);
        assert(fn(std::vector<Point>{{0, 0}, {0, 0}}) == 0);      // duplicate points
        assert(fn(std::vector<Point>{{0, 0}, {1, 0}, {2, 0}}) == 2);   // collinear
    };
    check([](const std::vector<Point>& p) { return min_cost_connect_prim(p); });
    check([](const std::vector<Point>& p) { return min_cost_connect_kruskal(p); });
    return 0;
}
