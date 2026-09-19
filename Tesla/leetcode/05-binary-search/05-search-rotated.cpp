// PROBLEM     Search for a target in a rotated sorted array of distinct values, O(log n).
// EXAMPLE     [4,5,6,7,0,1,2], target 0 -> index 4
// APPROACH    At any midpoint, AT LEAST ONE HALF IS SORTED (a single rotation cannot
//             break both). Determine which half is sorted by comparing v[lo] with v[mid],
//             then check whether the target lies inside that sorted half's range: if yes,
//             search there; if no, search the other half. Each step still halves the
//             range, so it stays O(log n).
// COMPLEXITY  Time O(log n), Space O(1).
// FOLLOW-UPS  Duplicates (LC 81)? -> when v[lo] == v[mid] == v[hi] you cannot tell which
//             half is sorted, so you shrink by one and the worst case degrades to O(n).
//             Alternative approach? -> find the pivot (problem 04), then binary search the
//             correct segment -- two clean O(log n) searches, and arguably easier to get
//             right under pressure. Say both and pick one.

#include <cassert>
#include <vector>

int search_rotated(const std::vector<int>& v, int target) {
    if (v.empty()) return -1;
    std::size_t lo = 0, hi = v.size() - 1;
    while (lo <= hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (v[mid] == target) return static_cast<int>(mid);

        if (v[lo] <= v[mid]) {                          // LEFT half is sorted
            if (v[lo] <= target && target < v[mid]) {
                if (mid == 0) break;
                hi = mid - 1;
            } else {
                lo = mid + 1;
            }
        } else {                                        // RIGHT half is sorted
            if (v[mid] < target && target <= v[hi]) {
                lo = mid + 1;
            } else {
                if (mid == 0) break;
                hi = mid - 1;
            }
        }
        if (lo > hi) break;                             // unsigned: avoid underflow
    }
    return -1;
}

int main() {
    const std::vector<int> v{4, 5, 6, 7, 0, 1, 2};
    assert(search_rotated(v, 0) == 4);
    assert(search_rotated(v, 4) == 0);                  // first element
    assert(search_rotated(v, 2) == 6);                  // last element
    assert(search_rotated(v, 7) == 3);                  // just before the pivot
    assert(search_rotated(v, 3) == -1);
    assert(search_rotated({}, 5) == -1);
    assert(search_rotated({1}, 1) == 0);
    assert(search_rotated({1}, 0) == -1);
    assert(search_rotated({1, 3}, 3) == 1);
    assert(search_rotated({3, 1}, 1) == 1);
    assert(search_rotated({1, 2, 3, 4, 5}, 4) == 3);    // not rotated
    return 0;
}
