// PROBLEM     A stack supporting push, pop, top and getMin, all in O(1).
// APPROACH    The trick is that the minimum is a function of the stack's HISTORY, so
//             store it alongside each element: each entry carries the minimum of the
//             stack up to and including itself. Popping restores the previous minimum for
//             free.
//             Space optimization worth mentioning: keep a second stack that only receives
//             a new value when it is <= the current minimum (ties matter -- if you use
//             '<' you lose the minimum when a duplicate is popped).
// COMPLEXITY  All operations O(1). Space O(n) either way; the two-stack version uses less
//             in practice when minima are rare.
// FOLLOW-UPS  getMax too? -> carry both. A QUEUE with O(1) min? -> two stacks, or a
//             monotonic deque (section 03, problem 06) -- a genuinely harder problem.
//             Thread safe? -> that is a different question: a lock, or a lock-free stack
//             with a CAS loop and the ABA problem (course section 11).

#include <cassert>
#include <optional>
#include <vector>

class MinStack {
    struct Entry { int value; int min_so_far; };
    std::vector<Entry> data_;
public:
    void push(int v) {
        const int m = data_.empty() ? v : std::min(v, data_.back().min_so_far);
        data_.push_back({v, m});
    }
    void pop() {
        assert(!data_.empty() && "pop on an empty MinStack");
        data_.pop_back();
    }
    // optional instead of UB or a magic value on an empty stack.
    std::optional<int> top() const {
        return data_.empty() ? std::nullopt : std::optional{data_.back().value};
    }
    std::optional<int> get_min() const {
        return data_.empty() ? std::nullopt : std::optional{data_.back().min_so_far};
    }
    bool empty() const noexcept { return data_.empty(); }
    std::size_t size() const noexcept { return data_.size(); }
};

int main() {
    MinStack s;
    assert(!s.top().has_value() && !s.get_min().has_value());

    s.push(-2); s.push(0); s.push(-3);
    assert(s.get_min() == -3);
    s.pop();
    assert(s.top() == 0);
    assert(s.get_min() == -2);
    s.pop(); s.pop();
    assert(s.empty());

    // Duplicated minima: popping one must NOT lose the minimum.
    MinStack d;
    d.push(1); d.push(1); d.push(2);
    assert(d.get_min() == 1);
    d.pop();
    assert(d.get_min() == 1);
    d.pop();
    assert(d.get_min() == 1);
    d.pop();
    assert(d.empty());
    return 0;
}
