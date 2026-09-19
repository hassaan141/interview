// PROBLEM     Return the k points closest to the origin.
// EXAMPLE     [[1,3],[-2,2]], k = 1 -> [[-2,2]]
// APPROACH    A bounded MAX-heap of size k on SQUARED distance: push each point, and when
//             the heap exceeds k, pop the farthest. Whatever remains is the k closest.
//             DO NOT CALL sqrt. It is monotonic, so it cannot change the ordering, and it
//             costs ~15-20 cycles plus a precision loss. Comparing x*x + y*y is exact for
//             integer inputs and much faster -- exactly the kind of thing a performance
//             oriented interviewer is listening for.
// COMPLEXITY  O(n log k) time, O(k) space. With the whole array in memory,
//             std::nth_element with a squared-distance comparator is O(n) -- offer both.
// FOLLOW-UPS  Streaming points? -> the bounded heap is the only option (O(k) memory).
//             Overflow? -> coordinates near INT_MAX square past int; use long long.
//             Many queries / spatial data? -> a k-d tree or a spatial hash, which is what
//             an actual perception stack uses for nearest-neighbour lookups.

#include <algorithm>
#include <array>
#include <cassert>
#include <queue>
#include <vector>

using Point = std::array<int, 2>;

static long long dist2(const Point& p) {          // squared: no sqrt, exact, cheap
    return static_cast<long long>(p[0]) * p[0] + static_cast<long long>(p[1]) * p[1];
}

// Bounded max-heap: O(n log k), O(k) space.
std::vector<Point> k_closest_heap(const std::vector<Point>& points, std::size_t k) {
    const auto farther = [](const Point& a, const Point& b) { return dist2(a) < dist2(b); };
    std::priority_queue<Point, std::vector<Point>, decltype(farther)> heap{farther};
    for (const Point& p : points) {
        heap.push(p);
        if (heap.size() > k) heap.pop();           // drop the farthest so far
    }
    std::vector<Point> out;
    out.reserve(heap.size());
    while (!heap.empty()) { out.push_back(heap.top()); heap.pop(); }
    return out;
}

// nth_element: O(n) average when the whole array is available.
std::vector<Point> k_closest_select(std::vector<Point> points, std::size_t k) {
    k = std::min(k, points.size());
    std::nth_element(points.begin(), points.begin() + static_cast<std::ptrdiff_t>(k),
                     points.end(),
                     [](const Point& a, const Point& b) { return dist2(a) < dist2(b); });
    points.resize(k);
    return points;
}

static std::vector<Point> sorted(std::vector<Point> v) { std::ranges::sort(v); return v; }

int main() {
    assert((sorted(k_closest_heap({{1, 3}, {-2, 2}}, 1)) == std::vector<Point>{{-2, 2}}));
    assert((sorted(k_closest_select({{1, 3}, {-2, 2}}, 1)) == std::vector<Point>{{-2, 2}}));

    const std::vector<Point> pts{{3, 3}, {5, -1}, {-2, 4}};
    assert(sorted(k_closest_heap(pts, 2)) == sorted(k_closest_select(pts, 2)));
    assert(k_closest_heap(pts, 2).size() == 2);

    assert(k_closest_heap({}, 3).empty());
    assert(k_closest_heap({{0, 0}}, 5).size() == 1);          // k larger than the input
    assert((k_closest_heap({{0, 0}}, 1) == std::vector<Point>{{0, 0}}));
    assert(k_closest_heap(pts, 0).empty());

    // Large coordinates: the long long squared distance is what keeps this correct.
    const std::vector<Point> big{{100000, 100000}, {1, 1}};
    assert((k_closest_heap(big, 1) == std::vector<Point>{{1, 1}}));
    return 0;
}
