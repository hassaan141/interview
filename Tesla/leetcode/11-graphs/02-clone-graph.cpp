// PROBLEM     Deep-copy a connected undirected graph given a node reference.
// APPROACH    Traverse (BFS or DFS) while maintaining a MAP from original node -> clone.
//             The map does double duty: it is the visited set AND the lookup that lets you
//             wire up an edge to a node you have already cloned. Create the clone on first
//             sight, then connect neighbours.
// COMPLEXITY  Time O(V + E), Space O(V) for the map plus the traversal structure.
// FOLLOW-UPS  Why does one map suffice? -> "already cloned" and "already visited" are the
//             same predicate here; a separate visited set is redundant.
//             Disconnected graph? -> the signature only gives one node, so you can only
//             reach its component. Say this; it is an assumption, not an oversight.
//             Cycles? -> handled for free: you never re-enqueue a node already in the map.
//             OWNERSHIP in C++ is the real interview question. A graph has cycles, so
//             unique_ptr children do not work and shared_ptr CYCLES LEAK (course section
//             10). The right answer for a real codebase is an ARENA: store the nodes in a
//             std::vector and connect them with indices -- no cycles in the ownership
//             graph, contiguous memory, trivially serializable. That is what the code
//             below does.

#include <cassert>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

struct GraphNode {
    int value{};
    std::vector<GraphNode*> neighbours;               // non-owning: the arena owns them
    explicit GraphNode(int v) : value{v} {}
};

// The arena owns every node. No ownership cycles, so nothing leaks (verify with ASan).
class GraphArena {
    std::vector<std::unique_ptr<GraphNode>> nodes_;
public:
    GraphNode* make(int value) {
        nodes_.push_back(std::make_unique<GraphNode>(value));
        return nodes_.back().get();
    }
    std::size_t size() const noexcept { return nodes_.size(); }
};

GraphNode* clone_graph(GraphNode* node, GraphArena& arena) {
    if (!node) return nullptr;
    std::unordered_map<GraphNode*, GraphNode*> clones;   // original -> clone (also "visited")
    clones[node] = arena.make(node->value);

    std::queue<GraphNode*> q;
    q.push(node);
    while (!q.empty()) {
        GraphNode* original = q.front();
        q.pop();
        for (GraphNode* neighbour : original->neighbours) {
            auto [it, inserted] = clones.try_emplace(neighbour, nullptr);
            if (inserted) {                              // first time we see it
                it->second = arena.make(neighbour->value);
                q.push(neighbour);
            }
            clones[original]->neighbours.push_back(it->second);
        }
    }
    return clones[node];
}

// Structural comparison, so the test verifies a real deep copy rather than a pointer.
static bool same_shape(GraphNode* a, GraphNode* b,
                       std::unordered_map<GraphNode*, GraphNode*>& seen) {
    if (!a || !b) return a == b;
    if (a == b) return false;                            // must NOT be the same object
    if (auto it = seen.find(a); it != seen.end()) return it->second == b;
    seen[a] = b;
    if (a->value != b->value) return false;
    if (a->neighbours.size() != b->neighbours.size()) return false;
    for (std::size_t i = 0; i < a->neighbours.size(); ++i)
        if (!same_shape(a->neighbours[i], b->neighbours[i], seen)) return false;
    return true;
}

int main() {
    GraphArena arena;
    // A 4-cycle with cross edges: 1-2-3-4-1, and 1-3, 2-4 absent.
    GraphNode* n1 = arena.make(1);
    GraphNode* n2 = arena.make(2);
    GraphNode* n3 = arena.make(3);
    GraphNode* n4 = arena.make(4);
    n1->neighbours = {n2, n4};
    n2->neighbours = {n1, n3};
    n3->neighbours = {n2, n4};
    n4->neighbours = {n1, n3};

    GraphArena clone_arena;
    GraphNode* copy = clone_graph(n1, clone_arena);
    std::unordered_map<GraphNode*, GraphNode*> seen;
    assert(copy != n1 && same_shape(n1, copy, seen));
    assert(clone_arena.size() == 4);

    // Mutating the original must not affect the clone -- that is what "deep" means.
    n1->value = 99;
    assert(copy->value == 1);

    assert(clone_graph(nullptr, clone_arena) == nullptr);

    GraphArena single;
    GraphNode* lone = single.make(7);
    GraphArena out;
    GraphNode* lone_copy = clone_graph(lone, out);
    assert(lone_copy != lone && lone_copy->value == 7 && lone_copy->neighbours.empty());

    GraphArena selfloop;
    GraphNode* s = selfloop.make(5);
    s->neighbours = {s};                                  // a self edge
    GraphArena out2;
    GraphNode* s_copy = clone_graph(s, out2);
    assert(s_copy != s && s_copy->neighbours.size() == 1 && s_copy->neighbours[0] == s_copy);
    return 0;
}
