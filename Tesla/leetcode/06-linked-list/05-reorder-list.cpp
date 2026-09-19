// PROBLEM     Reorder L0->L1->...->Ln-1->Ln into L0->Ln->L1->Ln-1->...  In place.
// EXAMPLE     1->2->3->4  =>  1->4->2->3
// APPROACH    COMPOSE three primitives you already know, which is the actual lesson:
//               1. find the middle              (fast/slow)
//               2. reverse the second half      (three-pointer reverse)
//               3. interleave the two halves    (alternating splice)
//             Splitting the list (null-terminating the first half) before reversing is
//             the step people forget -- without it you build a cycle.
// COMPLEXITY  Time O(n), Space O(1). The "copy into a vector and index from both ends"
//             version is O(n) space but far easier to write; say you know both and why
//             the O(1) one is preferred.
// FOLLOW-UPS  Palindrome linked list (LC 234)? -> middle + reverse + compare, same three
//             primitives. Odd vs even length? -> the fast/slow loop below gives the
//             FIRST middle for even lengths, which keeps the first half >= the second.

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

static Node* reverse(Node* head) {
    Node* prev = nullptr;
    while (head) { Node* next = head->next; head->next = prev; prev = head; head = next; }
    return prev;
}

void reorder_list(Node* head) {
    if (!head || !head->next) return;

    // 1. Middle: for even lengths this leaves `slow` at the END of the first half.
    Node* slow = head;
    Node* fast = head;
    while (fast->next && fast->next->next) { slow = slow->next; fast = fast->next->next; }

    // 2. Split (the step people forget) and reverse the second half.
    Node* second = reverse(slow->next);
    slow->next = nullptr;                     // terminate the first half

    // 3. Interleave.
    Node* first = head;
    while (second) {
        Node* n1 = first->next;
        Node* n2 = second->next;
        first->next = second;
        second->next = n1;
        first = n1;
        second = n2;
    }
}

static std::vector<int> to_vector(const Node* head) {
    std::vector<int> out;
    for (const Node* n = head; n; n = n->next) out.push_back(n->value);
    return out;
}

int main() {
    Arena arena;
    Node* a = arena.from({1, 2, 3, 4});
    reorder_list(a);
    assert(to_vector(a) == std::vector<int>({1, 4, 2, 3}));

    Node* b = arena.from({1, 2, 3, 4, 5});
    reorder_list(b);
    assert(to_vector(b) == std::vector<int>({1, 5, 2, 4, 3}));

    Node* c = arena.from({1});
    reorder_list(c);
    assert(to_vector(c) == std::vector<int>({1}));

    Node* d = arena.from({1, 2});
    reorder_list(d);
    assert(to_vector(d) == std::vector<int>({1, 2}));

    reorder_list(nullptr);                      // must not crash
    return 0;
}
