// PROBLEM     Remove the nth node from the end of the list, in ONE pass.
// EXAMPLE     1->2->3->4->5, n = 2  =>  1->2->3->5
// APPROACH    Two pointers n+1 apart, plus a DUMMY HEAD so removing the first node needs
//             no special case. Advance `fast` n+1 steps, then advance both until fast is
//             null; `slow` now points at the node BEFORE the one to remove.
// COMPLEXITY  Time O(n) in one pass, Space O(1). The two-pass version (count, then walk)
//             is just as good asymptotically -- the "one pass" constraint is the point.
// FOLLOW-UPS  n larger than the list? -> decide and STATE the behaviour (here: no-op).
//             Who deletes the removed node? -> in a real codebase, ownership must be
//             explicit. With unique_ptr the unlink IS the delete; with raw pointers you
//             must delete it or leak. LeetCode ignores this; an interviewer will not.
//             Doubly linked? -> removal is O(1) given the node, no traversal at all,
//             which is exactly why an LRU cache uses one (problem 06).

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

Node* remove_nth_from_end(Node* head, int n) {
    Node dummy{0, head};                    // sentinel: removing the head is not special
    Node* fast = &dummy;
    Node* slow = &dummy;

    for (int i = 0; i <= n; ++i) {          // n+1 steps so slow lands BEFORE the target
        if (!fast) return head;             // n is larger than the list: no-op
        fast = fast->next;
    }
    while (fast) { fast = fast->next; slow = slow->next; }

    Node* removed = slow->next;
    if (removed) slow->next = removed->next;   // the arena still owns the node
    return dummy.next;
}

static std::vector<int> to_vector(const Node* head) {
    std::vector<int> out;
    for (const Node* n = head; n; n = n->next) out.push_back(n->value);
    return out;
}

int main() {
    Arena arena;
    assert(to_vector(remove_nth_from_end(arena.from({1, 2, 3, 4, 5}), 2))
           == std::vector<int>({1, 2, 3, 5}));
    assert(remove_nth_from_end(arena.from({1}), 1) == nullptr);        // removes the head
    assert(to_vector(remove_nth_from_end(arena.from({1, 2}), 1)) == std::vector<int>({1}));
    assert(to_vector(remove_nth_from_end(arena.from({1, 2}), 2)) == std::vector<int>({2}));
    assert(to_vector(remove_nth_from_end(arena.from({1, 2, 3}), 5))
           == std::vector<int>({1, 2, 3}));                            // n too large
    assert(remove_nth_from_end(nullptr, 1) == nullptr);
    return 0;
}
