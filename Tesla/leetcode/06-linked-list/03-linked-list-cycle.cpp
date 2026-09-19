// PROBLEM     Does the list contain a cycle? If so, return the node where it begins.
// APPROACH    FLOYD'S TORTOISE AND HARE. slow advances 1, fast advances 2. If there is a
//             cycle they must meet inside it (the gap closes by 1 each step, modulo the
//             cycle length). If fast reaches null, there is no cycle.
//             To find the ENTRY: after they meet, reset one pointer to the head and
//             advance BOTH one step at a time; they meet at the cycle entry.
//             Why: let L = distance head->entry, C = cycle length, k = distance
//             entry->meeting point. When they meet, slow has walked L + k and fast
//             2(L+k), and the difference 2(L+k) - (L+k) = L + k is a multiple of C.
//             So L = mC - k, i.e. walking L more steps from the meeting point lands
//             exactly on the entry. Be able to say that -- it is the only interesting
//             part of the question.
// COMPLEXITY  Time O(n), Space O(1). The hash-set version is O(n) time and O(n) SPACE,
//             which is the trade you are being asked to avoid.
// FOLLOW-UPS  Cycle LENGTH? -> keep walking from the meeting point until you return.
//             Find the duplicate number (LC 287)? -> the SAME algorithm, treating the
//             array as a function i -> nums[i]. That connection is worth knowing.
//             Real systems: this is how you detect a corrupted circular buffer or a
//             self-referencing parent chain without allocating.

#include <cassert>
#include <memory>
#include <vector>

struct Node { int value{}; Node* next{nullptr}; };

class Arena {
    std::vector<std::unique_ptr<Node>> nodes_;
public:
    // Builds a list; if cycle_at >= 0, links the tail back to that index.
    Node* from(const std::vector<int>& values, int cycle_at = -1) {
        std::vector<Node*> raw;
        Node dummy{}; Node* tail = &dummy;
        for (int v : values) {
            nodes_.push_back(std::make_unique<Node>(Node{v, nullptr}));
            raw.push_back(nodes_.back().get());
            tail->next = raw.back();
            tail = tail->next;
        }
        if (cycle_at >= 0 && !raw.empty()) tail->next = raw[static_cast<std::size_t>(cycle_at)];
        return dummy.next;
    }
};

bool has_cycle(const Node* head) {
    const Node* slow = head;
    const Node* fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return true;
    }
    return false;
}

Node* detect_cycle_entry(Node* head) {
    Node* slow = head;
    Node* fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) {                  // they met: now find the entry
            Node* probe = head;
            while (probe != slow) { probe = probe->next; slow = slow->next; }
            return probe;
        }
    }
    return nullptr;
}

int main() {
    Arena arena;
    assert(!has_cycle(nullptr));
    assert(!has_cycle(arena.from({1})));
    assert(!has_cycle(arena.from({1, 2, 3})));

    Node* self_loop = arena.from({1}, 0);
    assert(has_cycle(self_loop) && detect_cycle_entry(self_loop) == self_loop);

    Node* c = arena.from({3, 2, 0, -4}, 1);          // tail -> index 1
    assert(has_cycle(c));
    assert(detect_cycle_entry(c) != nullptr && detect_cycle_entry(c)->value == 2);

    Node* full = arena.from({1, 2}, 0);               // whole list is the cycle
    assert(has_cycle(full) && detect_cycle_entry(full) == full);

    assert(detect_cycle_entry(arena.from({1, 2, 3})) == nullptr);
    assert(detect_cycle_entry(nullptr) == nullptr);
    return 0;
}
