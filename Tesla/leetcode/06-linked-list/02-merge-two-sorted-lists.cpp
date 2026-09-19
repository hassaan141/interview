// PROBLEM     Merge two sorted linked lists into one sorted list, splicing nodes (no new
//             allocation).
// EXAMPLE     1->2->4 and 1->3->4  =>  1->1->2->3->4->4
// APPROACH    A DUMMY HEAD plus a tail pointer removes every "is this the first node"
//             special case. Compare the two cursors, splice the smaller, advance. When
//             one list runs out, attach the rest of the other in O(1) -- you do not need
//             to walk it.
// COMPLEXITY  Time O(m + n), Space O(1) -- nodes are relinked, not copied.
// FOLLOW-UPS  Stability: use <= so equal elements keep list-a order.
//             Merge K lists (LC 23)? -> either a min-heap over the k heads (O(N log k),
//             section 09) or pairwise merging in a tournament (also O(N log k), better
//             constants and no heap). Say both.
//             Why is this the core of merge sort on lists? -> because splicing is O(1),
//             list merge sort is O(n log n) time and O(log n) stack, with NO extra array.

#include <cassert>
#include <memory>
#include <vector>

struct Node { int value{}; Node* next{nullptr}; };

class Arena {
    std::vector<std::unique_ptr<Node>> nodes_;
public:
    Node* from(const std::vector<int>& values) {
        Node dummy{}; Node* tail = &dummy;
        for (int v : values) {
            nodes_.push_back(std::make_unique<Node>(Node{v, nullptr}));
            tail->next = nodes_.back().get();
            tail = tail->next;
        }
        return dummy.next;
    }
};

Node* merge_two_lists(Node* a, Node* b) {
    Node dummy{};                        // stack-allocated sentinel: no allocation
    Node* tail = &dummy;
    while (a && b) {
        if (a->value <= b->value) { tail->next = a; a = a->next; }   // <= keeps it stable
        else                      { tail->next = b; b = b->next; }
        tail = tail->next;
    }
    tail->next = a ? a : b;              // attach the remainder in O(1)
    return dummy.next;
}

static std::vector<int> to_vector(const Node* head) {
    std::vector<int> out;
    for (const Node* n = head; n; n = n->next) out.push_back(n->value);
    return out;
}

int main() {
    Arena arena;
    assert(to_vector(merge_two_lists(arena.from({1, 2, 4}), arena.from({1, 3, 4})))
           == std::vector<int>({1, 1, 2, 3, 4, 4}));
    assert(merge_two_lists(nullptr, nullptr) == nullptr);
    assert(to_vector(merge_two_lists(nullptr, arena.from({0}))) == std::vector<int>({0}));
    assert(to_vector(merge_two_lists(arena.from({5}), nullptr)) == std::vector<int>({5}));
    assert(to_vector(merge_two_lists(arena.from({1, 2, 3}), arena.from({4, 5, 6})))
           == std::vector<int>({1, 2, 3, 4, 5, 6}));           // disjoint ranges
    assert(to_vector(merge_two_lists(arena.from({4, 5, 6}), arena.from({1, 2, 3})))
           == std::vector<int>({1, 2, 3, 4, 5, 6}));
    assert(to_vector(merge_two_lists(arena.from({2, 2}), arena.from({2, 2})))
           == std::vector<int>({2, 2, 2, 2}));                 // all equal
    return 0;
}
