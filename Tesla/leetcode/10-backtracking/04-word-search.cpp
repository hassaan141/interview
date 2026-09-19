// PROBLEM     Does `word` exist in the grid as a path of adjacent (4-directional) cells,
//             using each cell at most once?
// EXAMPLE     board [[A,B,C,E],[S,F,C,S],[A,D,E,E]], word "ABCCED" -> true
// APPROACH    DFS from every cell, matching one character per step. Mark the cell as
//             visited before recursing and RESTORE it afterwards -- mutating the board in
//             place avoids a separate visited array and the allocation that comes with it.
//             Prune the moment the character does not match.
// COMPLEXITY  Time O(cells * 4^L) worst case with L = word length (3^L after the first
//             step, since you never immediately go back). Space O(L) recursion.
// FOLLOW-UPS  MANY words? -> do not call this per word; build a trie and search them all
//             in one traversal (section 08, problem 03).
//             Early rejection? -> if the board's character counts cannot cover the word's,
//             fail immediately; and if the word's last letter is rarer than its first,
//             search the word REVERSED. Both are cheap and dramatic.
//             Why restore the cell? -> a different path may legitimately reuse it; not
//             restoring is the classic bug and produces false negatives.

#include <array>
#include <cassert>
#include <string_view>
#include <vector>

namespace {
bool dfs(std::vector<std::vector<char>>& board, std::size_t r, std::size_t c,
         std::string_view word, std::size_t i) {
    if (i == word.size()) return true;
    if (r >= board.size() || c >= board[0].size()) return false;   // unsigned wrap catches <0
    if (board[r][c] != word[i]) return false;                       // prune

    const char saved = board[r][c];
    board[r][c] = '\0';                                             // mark visited
    const bool found = dfs(board, r + 1, c, word, i + 1) ||
                       dfs(board, r - 1, c, word, i + 1) ||          // wraps if r == 0
                       dfs(board, r, c + 1, word, i + 1) ||
                       dfs(board, r, c - 1, word, i + 1);
    board[r][c] = saved;                                            // RESTORE
    return found;
}
}  // namespace

bool exist(std::vector<std::vector<char>> board, std::string_view word) {
    if (word.empty()) return true;
    if (board.empty() || board[0].empty()) return false;

    // Cheap early rejection: the board cannot contain more of a letter than it has.
    std::array<int, 128> board_count{}, word_count{};
    for (const auto& row : board) for (char ch : row) ++board_count[static_cast<unsigned char>(ch)];
    for (char ch : word) ++word_count[static_cast<unsigned char>(ch)];
    for (std::size_t i = 0; i < 128; ++i)
        if (word_count[i] > board_count[i]) return false;

    for (std::size_t r = 0; r < board.size(); ++r)
        for (std::size_t c = 0; c < board[0].size(); ++c)
            if (dfs(board, r, c, word, 0)) return true;
    return false;
}

int main() {
    const std::vector<std::vector<char>> board{
        {'A', 'B', 'C', 'E'},
        {'S', 'F', 'C', 'S'},
        {'A', 'D', 'E', 'E'}};
    assert(exist(board, "ABCCED"));
    assert(exist(board, "SEE"));
    assert(!exist(board, "ABCB"));            // would need to reuse 'B'
    assert(exist(board, "A"));
    assert(!exist(board, "Z"));
    assert(exist(board, ""));
    assert(!exist({}, "A"));
    assert(exist({{'A'}}, "A"));
    assert(!exist({{'A'}}, "AA"));            // only one cell
    assert(!exist(board, "ABCESEEEFS"));      // too long for the available letters
    return 0;
}
