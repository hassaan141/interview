// PROBLEM     Reverse a singly linked list.
// EXAMPLE     1->2->3->4->5  =>  5->4->3->2->1
// APPROACH    Three pointers: prev, curr, next. Save next BEFORE rewriting curr->next,
//             or you lose the rest of the list. That single line is the whole problem.
// COMPLEXITY  Time O(n), Space O(1). The recursive version is O(n) space (stack) and
//             will overflow on a long list -- mention that, it is the real-world point.
// FOLLOW-UPS  Reverse between positions m..n (LC 92)? -> same loop with a dummy head and
//             a saved "node before m".
//             Reverse in k-groups (LC 25)? -> count k ahead, reverse, splice, repeat.
//             OWNERSHIP: with unique_ptr<Node> next, reversal has to move the pointers,
//             and the DESTRUCTOR recurses once per node -- a 1e6-node list blows the
//             stack. The iterative teardown below is the fix, and saying it unprompted is
//             what distinguishes a C++ answer from a LeetCode answer.

#include <cassert>
#include <memory>
#include <utility>
#include <vector>

struct Node {
    int value{};
    Node* next{nullptr};
};

// A tiny arena so the tests do not leak and there is no per-node allocation.
// This is also the layout a real system would use: contiguous, cache friendly.
class Arena {
    std::vector<std::unique_ptr<Node>> nodes_;
public:
    Node* make(int v) { nodes_.push_back(std::make_unique<Node>(Node{v, nullptr})); return nodes_.back().get(); }
    Node* from(const std::vector<int>& values) {
        Node dummy{}; Node* tail = &dummy;
        for (int v : values) { tail->next = make(v); tail = tail->next; }
        return dummy.next;
    }
};

Node* reverse_list(Node* head) {
    Node* prev = nullptr;
    while (head) {
        Node* next = head->next;      // SAVE FIRST -- everything else depends on this
        head->next = prev;
        prev = head;
        head = next;
    }
    return prev;                       // the old tail is the new head
}

// Recursive: elegant, and a stack-overflow waiting to happen. Know both.
Node* reverse_list_recursive(Node* head) {
    if (!head || !head->next) return head;
    Node* new_head = reverse_list_recursive(head->next);
    head->next->next = head;           // the node after us now points back at us
    head->next = nullptr;
    return new_head;
}

static std::vector<int> to_vector(const Node* head) {
    std::vector<int> out;
    for (const Node* n = head; n; n = n->next) out.push_back(n->value);
    return out;
}

// The owning version, and the iterative teardown that stops the destructor recursing.
struct OwningNode {
    int value{};
    std::unique_ptr<OwningNode> next;
    ~OwningNode() {
        // Without this, destroying a long chain recurses once per node and overflows.
        while (next) next = std::move(next->next);
    }
};

int main() {
    Arena arena;
    assert(to_vector(reverse_list(arena.from({1, 2, 3, 4, 5})))
           == std::vector<int>({5, 4, 3, 2, 1}));
    assert(to_vector(reverse_list(arena.from({1, 2}))) == std::vector<int>({2, 1}));
    assert(to_vector(reverse_list(arena.from({1}))) == std::vector<int>({1}));
    assert(reverse_list(nullptr) == nullptr);

    assert(to_vector(reverse_list_recursive(arena.from({1, 2, 3})))
           == std::vector<int>({3, 2, 1}));

    // Reversing twice is the identity -- a cheap property test.
    Node* twice = reverse_list(reverse_list(arena.from({4, 7, 9})));
    assert(to_vector(twice) == std::vector<int>({4, 7, 9}));

    // 100k owning nodes: this destructs without overflowing the stack.
    {
        auto head = std::make_unique<OwningNode>();
        OwningNode* tail = head.get();
        for (int i = 0; i < 100000; ++i) {
            tail->next = std::make_unique<OwningNode>();
            tail = tail->next.get();
        }
    }
    return 0;
}
