// PROBLEM     Place n queens on an n x n board so none attack each other. Return all
//             distinct solutions (and, separately, just the count).
// APPROACH    Place one queen per ROW, so rows are handled by construction. Maintain three
//             O(1) membership structures for the constraints:
//               - column c              -> cols[c]
//               - "/" diagonal          -> r + c                (constant along it)
//               - "\" diagonal          -> r - c + (n - 1)      (shifted to stay >= 0)
//             Try each column in the current row, skip if any constraint is violated,
//             recurse, then undo. The constant-time conflict check is the entire
//             optimization -- rescanning the board would be O(n) per placement.
// COMPLEXITY  O(n!) in the worst case, far less with pruning. Space O(n).
// FOLLOW-UPS  Count only (LC 52)? -> drop the board construction; that alone is a large
//             constant-factor win, which is why it is a separate problem.
//             BITMASKS: cols, diag1 and diag2 as integers, and the available squares are
//             `~(cols | d1 | d2) & mask`; then `x & -x` picks the lowest set bit. That is
//             the version that solves n=15 quickly, and it is worth mentioning because it
//             is the same bit-trick vocabulary as section 16.
//             Symmetry? -> only search half the first row and mirror the results.

#include <cassert>
#include <string>
#include <vector>

namespace {
void solve(int n, int row, std::vector<int>& queen_col, std::vector<bool>& cols,
           std::vector<bool>& diag1, std::vector<bool>& diag2,
           std::vector<std::vector<std::string>>& out, int& count) {
    if (row == n) {
        ++count;
        std::vector<std::string> board;
        board.reserve(static_cast<std::size_t>(n));
        for (int r = 0; r < n; ++r) {
            std::string line(static_cast<std::size_t>(n), '.');
            line[static_cast<std::size_t>(queen_col[static_cast<std::size_t>(r)])] = 'Q';
            board.push_back(std::move(line));
        }
        out.push_back(std::move(board));           // build the board ONLY at a leaf
        return;
    }
    for (int c = 0; c < n; ++c) {
        const auto d1 = static_cast<std::size_t>(row + c);
        const auto d2 = static_cast<std::size_t>(row - c + n - 1);
        if (cols[static_cast<std::size_t>(c)] || diag1[d1] || diag2[d2]) continue;  // prune

        cols[static_cast<std::size_t>(c)] = diag1[d1] = diag2[d2] = true;
        queen_col[static_cast<std::size_t>(row)] = c;
        solve(n, row + 1, queen_col, cols, diag1, diag2, out, count);
        cols[static_cast<std::size_t>(c)] = diag1[d1] = diag2[d2] = false;          // undo
    }
}
}  // namespace

std::vector<std::vector<std::string>> solve_n_queens(int n) {
    std::vector<std::vector<std::string>> out;
    if (n <= 0) return out;
    std::vector<int> queen_col(static_cast<std::size_t>(n), 0);
    std::vector<bool> cols(static_cast<std::size_t>(n), false);
    std::vector<bool> diag1(static_cast<std::size_t>(2 * n - 1), false);
    std::vector<bool> diag2(static_cast<std::size_t>(2 * n - 1), false);
    int count = 0;
    solve(n, 0, queen_col, cols, diag1, diag2, out, count);
    return out;
}

// Count only, with bitmasks: no board construction, no vectors, no allocation.
int total_n_queens(int n) {
    const int full = (1 << n) - 1;
    int count = 0;
    // Recursive lambda (pre-C++23 style): pass yourself in.
    const auto go = [&](auto&& self, int cols, int d1, int d2) -> void {
        if (cols == full) { ++count; return; }
        int available = ~(cols | d1 | d2) & full;
        while (available) {
            const int bit = available & -available;      // lowest set bit
            available -= bit;
            self(self, cols | bit, (d1 | bit) << 1 & full, (d2 | bit) >> 1);
        }
    };
    if (n > 0) go(go, 0, 0, 0);
    return count;
}

int main() {
    assert(solve_n_queens(4).size() == 2);
    assert((solve_n_queens(4)[0] ==
            std::vector<std::string>{".Q..", "...Q", "Q...", "..Q."}));
    assert(solve_n_queens(1).size() == 1);
    assert(solve_n_queens(2).empty());                    // impossible
    assert(solve_n_queens(3).empty());                    // impossible
    assert(solve_n_queens(0).empty());
    assert(solve_n_queens(8).size() == 92);               // the classic answer

    for (int n = 0; n <= 8; ++n)
        assert(total_n_queens(n) == static_cast<int>(solve_n_queens(n).size()));
    assert(total_n_queens(10) == 724);                    // the fast version scales
    return 0;
}
