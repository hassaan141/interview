// PROBLEM     An m x n matrix where each row is sorted and the first element of each row
//             is greater than the last of the previous row. Search for a target in
//             O(log(m*n)).
// EXAMPLE     [[1,3,5,7],[10,11,16,20],[23,30,34,60]], target 3 -> true
// APPROACH    The constraint means the matrix is one sorted sequence laid out row-major.
//             So binary search over the flat index i in [0, m*n) and map it back with
//             row = i / n, col = i % n. No two-stage search needed.
// COMPLEXITY  Time O(log(m*n)), Space O(1).
// FOLLOW-UPS  The WEAKER version (LC 240): rows and columns sorted but rows not chained.
//             Then flattening is invalid. Start at the top-right corner and walk: move
//             left if too big, down if too small -- O(m + n). Both are below.
//             Why is the staircase walk optimal for LC 240? -> each step eliminates a
//             whole row or column; you cannot do better than O(m+n) in the worst case.

#include <cassert>
#include <vector>

// Fully sorted when flattened.
bool search_matrix(const std::vector<std::vector<int>>& m, int target) {
    if (m.empty() || m[0].empty()) return false;
    const std::size_t rows = m.size(), cols = m[0].size();
    std::size_t lo = 0, hi = rows * cols;              // flat, half-open
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        const int value = m[mid / cols][mid % cols];    // the only interesting line
        if (value == target) return true;
        if (value < target) lo = mid + 1;
        else                hi = mid;
    }
    return false;
}

// LC 240: rows sorted left-to-right, columns sorted top-to-bottom, NOT chained.
bool search_matrix_ii(const std::vector<std::vector<int>>& m, int target) {
    if (m.empty() || m[0].empty()) return false;
    // Start top-right: the only corner where the two directions disagree, so each
    // comparison eliminates an entire row or column.
    std::size_t row = 0, col = m[0].size() - 1;
    while (row < m.size()) {
        const int value = m[row][col];
        if (value == target) return true;
        if (value > target) { if (col == 0) return false; --col; }   // too big: go left
        else                ++row;                                    // too small: go down
    }
    return false;
}

int main() {
    const std::vector<std::vector<int>> m{{1, 3, 5, 7}, {10, 11, 16, 20}, {23, 30, 34, 60}};
    assert(search_matrix(m, 3));
    assert(search_matrix(m, 1));
    assert(search_matrix(m, 60));
    assert(!search_matrix(m, 13));
    assert(!search_matrix(m, 0));
    assert(!search_matrix(m, 61));
    assert(!search_matrix({}, 1));
    assert(!search_matrix({{}}, 1));
    assert(search_matrix({{5}}, 5));

    const std::vector<std::vector<int>> m2{{1, 4, 7, 11}, {2, 5, 8, 12}, {3, 6, 9, 16}};
    assert(search_matrix_ii(m2, 5));
    assert(!search_matrix_ii(m2, 10));
    assert(search_matrix_ii(m2, 1));
    assert(search_matrix_ii(m2, 16));
    assert(!search_matrix_ii(m2, 0));
    return 0;
}
