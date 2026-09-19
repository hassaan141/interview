// PROBLEM     Support addNum(x) and findMedian() over a stream of numbers.
// APPROACH    TWO HEAPS. A max-heap holds the lower half, a min-heap the upper half, and
//             the invariant is:
//               - every element of `lower` <= every element of `upper`
//               - sizes differ by at most one, with `lower` allowed to be the bigger
//             Then the median is lower.top() (odd total) or the average of the two tops
//             (even). Insert into one, then move the top across to restore the ordering,
//             then rebalance the sizes. Doing those two steps in that order is what makes
//             it correct without case analysis.
// COMPLEXITY  addNum O(log n), findMedian O(1), space O(n).
// FOLLOW-UPS  Values in a small fixed range (e.g. 0-100)? -> a counting array gives O(1)
//             add and O(range) median, far better constants. Interviewers ask this.
//             A SLIDING-window median? -> two heaps do not support arbitrary erase; use an
//             ordered multiset (std::multiset with an iterator to the middle) or lazy
//             deletion. Say that the heaps break down here.
//             Only need a percentile approximately? -> a t-digest or an HdrHistogram --
//             which is what you would actually use for p99 latency in the evaluation
//             pipeline (course sections 13, 14).

#include <cassert>
#include <cmath>
#include <optional>
#include <queue>
#include <vector>

class MedianFinder {
    std::priority_queue<int> lower_;                                       // max-heap
    std::priority_queue<int, std::vector<int>, std::greater<>> upper_;     // min-heap
public:
    void add_num(int x) {
        lower_.push(x);                       // 1. always insert into the lower half
        upper_.push(lower_.top());            // 2. move its top across: restores ordering
        lower_.pop();
        if (upper_.size() > lower_.size()) {  // 3. rebalance sizes (lower may be bigger)
            lower_.push(upper_.top());
            upper_.pop();
        }
    }
    std::optional<double> find_median() const {
        if (lower_.empty()) return std::nullopt;
        if (lower_.size() > upper_.size()) return lower_.top();
        return (static_cast<double>(lower_.top()) + upper_.top()) / 2.0;
    }
    std::size_t size() const noexcept { return lower_.size() + upper_.size(); }
};

static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

int main() {
    MedianFinder m;
    assert(!m.find_median().has_value());          // empty: no median

    m.add_num(1);
    assert(near(*m.find_median(), 1.0));
    m.add_num(2);
    assert(near(*m.find_median(), 1.5));
    m.add_num(3);
    assert(near(*m.find_median(), 2.0));

    MedianFinder d;                                 // descending input
    for (int x : {5, 4, 3, 2, 1}) d.add_num(x);
    assert(near(*d.find_median(), 3.0));

    MedianFinder e;                                 // duplicates
    for (int i = 0; i < 4; ++i) e.add_num(7);
    assert(near(*e.find_median(), 7.0));

    MedianFinder n;                                 // negatives, even count
    for (int x : {-5, -1, -3, -2}) n.add_num(x);
    assert(near(*n.find_median(), -2.5));
    assert(n.size() == 4);
    return 0;
}
