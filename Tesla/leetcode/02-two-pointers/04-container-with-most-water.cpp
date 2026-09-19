// PROBLEM     Given heights, pick two lines that with the x-axis hold the most water.
//             area = (hi - lo) * min(height[lo], height[hi]).
// EXAMPLE     [1,8,6,2,5,4,8,3,7] -> 49
// APPROACH    Start at the widest pair and move inward, always moving the SHORTER wall.
//             PROOF (say this out loud): the area is capped by the shorter wall. Moving
//             the taller wall inward strictly reduces the width and cannot raise the cap,
//             so every pair it would have formed is <= the current area. Therefore
//             discarding the shorter wall skips nothing.
// COMPLEXITY  Time O(n), Space O(1). Brute force is O(n^2).
// FOLLOW-UPS  Why not move both? -> you would skip pairs. Equal heights? -> either move
//             is safe. Trapping Rain Water is a DIFFERENT problem on the same input --
//             see 05, and be able to say how they differ (max single container vs. total
//             water held between all bars).

#include <algorithm>
#include <cassert>
#include <vector>

int max_area(const std::vector<int>& height) {
    if (height.size() < 2) return 0;
    std::size_t lo = 0, hi = height.size() - 1;
    int best = 0;
    while (lo < hi) {
        const int h = std::min(height[lo], height[hi]);
        best = std::max(best, static_cast<int>(hi - lo) * h);
        if (height[lo] < height[hi]) ++lo;           // move the SHORTER wall
        else                         --hi;
    }
    return best;
}

int main() {
    assert(max_area({1, 8, 6, 2, 5, 4, 8, 3, 7}) == 49);
    assert(max_area({1, 1}) == 1);
    assert(max_area({4, 3, 2, 1, 4}) == 16);         // equal ends, far apart
    assert(max_area({1, 2, 1}) == 2);
    assert(max_area({}) == 0);
    assert(max_area({5}) == 0);                       // need two walls
    assert(max_area({0, 0}) == 0);
    assert(max_area({2, 3, 4, 5, 18, 17, 6}) == 17);
    return 0;
}
