// PROBLEM     Sort an array of 0s, 1s and 2s in place, in ONE pass, with O(1) space.
// EXAMPLE     [2,0,2,1,1,0] -> [0,0,1,1,2,2]
// APPROACH    Dutch National Flag (Dijkstra). THREE pointers maintain the invariant:
//               [0, low)      -> all 0s
//               [low, mid)    -> all 1s
//               [mid, high]   -> unexamined
//               (high, n)     -> all 2s
//             On seeing a 0: swap into the 0-region and advance both low and mid. On a 1:
//             just advance mid. On a 2: swap to the back and shrink high WITHOUT advancing
//             mid, because the swapped-in value has not been examined yet.
// COMPLEXITY  Time O(n) with one pass, Space O(1). The counting-sort answer is also O(n)
//             but needs two passes; the question usually says one.
// FOLLOW-UPS  Why is mid not advanced on a 2? -> the element swapped in from the back is
//             unexamined; advancing would skip it. This is THE bug in this problem.
//             k colors? -> counting sort, O(n + k). Generalization? -> this is the
//             three-way partition used by quicksort on inputs with many duplicates.

#include <algorithm>
#include <cassert>
#include <vector>

void sort_colors(std::vector<int>& v) {
    std::size_t low = 0, mid = 0;
    std::size_t high = v.size();                    // exclusive, so empty input is safe
    while (mid < high) {
        if (v[mid] == 0)      std::swap(v[low++], v[mid++]);
        else if (v[mid] == 1) ++mid;
        else                  std::swap(v[mid], v[--high]);   // do NOT advance mid
    }
}

int main() {
    std::vector<int> a{2, 0, 2, 1, 1, 0};
    sort_colors(a);
    assert((a == std::vector<int>{0, 0, 1, 1, 2, 2}));

    std::vector<int> b{2, 0, 1};
    sort_colors(b);
    assert((b == std::vector<int>{0, 1, 2}));

    std::vector<int> c{};
    sort_colors(c);
    assert(c.empty());

    std::vector<int> d{1};
    sort_colors(d);
    assert((d == std::vector<int>{1}));

    std::vector<int> e{2, 2, 2};
    sort_colors(e);
    assert((e == std::vector<int>{2, 2, 2}));

    std::vector<int> f{0, 0, 0};
    sort_colors(f);
    assert((f == std::vector<int>{0, 0, 0}));

    std::vector<int> g{2, 1, 0, 2, 1, 0, 1};
    sort_colors(g);
    assert(std::ranges::is_sorted(g));
    return 0;
}
