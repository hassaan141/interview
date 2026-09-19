// PROBLEM     Given a grid of letters and a list of words, return every word that can be
//             formed by a path of adjacent cells (no cell reused within one word).
// APPROACH    Do NOT run a DFS per word (O(words * cells * 4^L)). Instead build a TRIE of
//             all the words and run ONE DFS from each cell, walking the grid and the trie
//             together. A path dies as soon as the prefix leaves the trie -- so all the
//             words are searched simultaneously and pruned in parallel.
//             Two details that matter:
//               - mark the cell visited before recursing and restore it after (backtracking)
//               - when a word is found, clear its marker and PRUNE leaf nodes on the way
//                 out, so the search does not keep re-exploring dead subtrees
// COMPLEXITY  Time O(cells * 4^L) worst case with L the longest word, but the trie pruning
//             makes it dramatically better in practice. Space O(total word characters).
// FOLLOW-UPS  Why is pruning necessary? -> without it, a grid of all 'a's with words
//             "a","aa","aaa",... re-walks the same subtrees; pruning makes it linear in
//             the found words.
//             Diagonal moves, or reusing cells? -> change the neighbour list / drop the
//             visited marker, and say how the complexity changes.

#include <array>
#include <cassert>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace {

struct Node {
    std::array<std::unique_ptr<Node>, 26> children{};
    int word_index = -1;                                 // >= 0 means "a word ends here"
    int child_count = 0;
};

void insert(Node* root, const std::string& word, int index) {
    Node* n = root;
    for (char c : word) {
        auto& child = n->children[static_cast<std::size_t>(c - 'a')];
        if (!child) { child = std::make_unique<Node>(); ++n->child_count; }
        n = child.get();
    }
    n->word_index = index;
}

void dfs(std::vector<std::vector<char>>& board, std::size_t r, std::size_t c, Node* node,
         const std::vector<std::string>& words, std::vector<std::string>& found) {
    const char ch = board[r][c];
    if (ch == '#') return;                               // already on the current path
    Node* next = node->children[static_cast<std::size_t>(ch - 'a')].get();
    if (!next) return;                                   // prefix not in the trie: prune

    if (next->word_index >= 0) {
        found.push_back(words[static_cast<std::size_t>(next->word_index)]);
        next->word_index = -1;                           // do not report it twice
    }

    board[r][c] = '#';                                   // mark visited
    const std::size_t rows = board.size(), cols = board[0].size();
    if (r > 0)        dfs(board, r - 1, c, next, words, found);
    if (r + 1 < rows) dfs(board, r + 1, c, next, words, found);
    if (c > 0)        dfs(board, r, c - 1, next, words, found);
    if (c + 1 < cols) dfs(board, r, c + 1, next, words, found);
    board[r][c] = ch;                                    // restore (backtrack)

    // PRUNE: a node with no remaining words and no children can never match again.
    if (next->word_index < 0 && next->child_count == 0) {
        node->children[static_cast<std::size_t>(ch - 'a')].reset();
        --node->child_count;
    }
}

}  // namespace

std::vector<std::string> find_words(std::vector<std::vector<char>> board,
                                    const std::vector<std::string>& words) {
    std::vector<std::string> found;
    if (board.empty() || board[0].empty()) return found;

    Node root;
    for (std::size_t i = 0; i < words.size(); ++i) insert(&root, words[i], static_cast<int>(i));

    for (std::size_t r = 0; r < board.size(); ++r)
        for (std::size_t c = 0; c < board[0].size(); ++c)
            dfs(board, r, c, &root, words, found);

    std::ranges::sort(found);
    return found;
}

int main() {
    const std::vector<std::vector<char>> board{
        {'o', 'a', 'a', 'n'},
        {'e', 't', 'a', 'e'},
        {'i', 'h', 'k', 'r'},
        {'i', 'f', 'l', 'v'}};
    assert((find_words(board, {"oath", "pea", "eat", "rain"})
            == std::vector<std::string>{"eat", "oath"}));

    assert(find_words({{'a', 'b'}, {'c', 'd'}}, {"abcb"}).empty());   // cannot reuse a cell
    assert((find_words({{'a'}}, {"a"}) == std::vector<std::string>{"a"}));
    assert(find_words({}, {"a"}).empty());
    assert(find_words({{'a'}}, {}).empty());

    // The pathological case that needs pruning to finish quickly.
    std::vector<std::vector<char>> aaa(6, std::vector<char>(6, 'a'));
    const auto hits = find_words(aaa, {"a", "aa", "aaa", "aaaa", "aaaaa"});
    assert(hits.size() == 5);
    return 0;
}
