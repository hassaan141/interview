// PROBLEM     A sorted array was rotated at an unknown pivot. Find the minimum in
//             O(log n). Assume distinct values first, then handle duplicates.
// EXAMPLE     [3,4,5,1,2] -> 1 ;  [4,5,6,7,0,1,2] -> 0 ;  [11,13,15,17] -> 11
// APPROACH    Compare the midpoint with the RIGHT end, not the left. If v[mid] > v[hi]
//             the minimum must be strictly right of mid; otherwise mid could be the
//             minimum, so keep it. Comparing with the left end does not work when the
//             array is not rotated at all -- that asymmetry is the whole trick.
// COMPLEXITY  Time O(log n), Space O(1).
// FOLLOW-UPS  DUPLICATES (LC 154)? -> when v[mid] == v[hi] you cannot tell which half
//             holds the minimum, so you can only shrink by one (--hi). That makes the
//             worst case O(n), e.g. [1,1,1,1,0,1]. Be able to state that the O(log n)
//             guarantee is LOST with duplicates -- interviewers ask precisely this.
//             How many times was it rotated? -> the index of the minimum.

#include <cassert>
#include <vector>

// Distinct values: guaranteed O(log n).
int find_min(const std::vector<int>& v) {
    assert(!v.empty());
    std::size_t lo = 0, hi = v.size() - 1;             // INCLUSIVE here: comparing ends
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (v[mid] > v[hi]) lo = mid + 1;              // minimum is strictly right
        else                hi = mid;                  // mid could BE the minimum
    }
    return v[lo];
}

// With duplicates: correct, but worst case O(n).
int find_min_with_duplicates(const std::vector<int>& v) {
    assert(!v.empty());
    std::size_t lo = 0, hi = v.size() - 1;
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (v[mid] > v[hi])      lo = mid + 1;
        else if (v[mid] < v[hi]) hi = mid;
        else                     --hi;   // ambiguous: can only discard one. O(n) worst.
    }
    return v[lo];
}

int main() {
    assert(find_min({3, 4, 5, 1, 2}) == 1);
    assert(find_min({4, 5, 6, 7, 0, 1, 2}) == 0);
    assert(find_min({11, 13, 15, 17}) == 11);          // not rotated at all
    assert(find_min({2, 1}) == 1);
    assert(find_min({1}) == 1);
    assert(find_min({5, 1, 2, 3, 4}) == 1);            // rotated by one
    assert(find_min({2, 3, 4, 5, 1}) == 1);

    assert(find_min_with_duplicates({2, 2, 2, 0, 1}) == 0);
    assert(find_min_with_duplicates({1, 1, 1, 1}) == 1);
    assert(find_min_with_duplicates({3, 3, 1, 3}) == 1);    // the O(n) shape
    assert(find_min_with_duplicates({1, 0, 1, 1, 1}) == 0);
    return 0;
}
