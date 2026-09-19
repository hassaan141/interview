// PROBLEM     (a) Level order traversal, grouped by level.
//             (b) Right side view: the last node of each level.
// APPROACH    BFS with a queue. The ONE trick: snapshot q.size() before the inner loop --
//             that count is exactly the current level, because the queue only grows with
//             the next level's nodes.
// COMPLEXITY  Time O(n), Space O(w) where w is the maximum width (up to n/2 for the last
//             level of a complete tree).
// FOLLOW-UPS  Right side view with DFS? -> preorder visiting RIGHT first, recording the
//             first node seen at each depth. O(h) space instead of O(w) -- a genuine
//             trade-off worth naming.
//             Zigzag (LC 103)? -> reverse alternate levels, or push to a deque from both
//             ends. Bottom-up (LC 107)? -> build normally, then reverse.
//             Why BFS and not DFS here? -> level grouping is a breadth property.

#include "tree.hpp"
#include <algorithm>
#include <cassert>
#include <vector>

std::vector<std::vector<int>> level_order(TreeNode* root) {
    std::vector<std::vector<int>> out;
    if (!root) return out;
    std::queue<TreeNode*> q;
    q.push(root);
    while (!q.empty()) {
        const std::size_t level_size = q.size();     // SNAPSHOT: this is one level
        std::vector<int> level;
        level.reserve(level_size);
        for (std::size_t i = 0; i < level_size; ++i) {
            TreeNode* n = q.front();
            q.pop();
            level.push_back(n->value);
            if (n->left)  q.push(n->left.get());
            if (n->right) q.push(n->right.get());
        }
        out.push_back(std::move(level));
    }
    return out;
}

// Last node of each level. BFS: O(w) space.
std::vector<int> right_side_view(TreeNode* root) {
    std::vector<int> out;
    if (!root) return out;
    std::queue<TreeNode*> q;
    q.push(root);
    while (!q.empty()) {
        const std::size_t level_size = q.size();
        for (std::size_t i = 0; i < level_size; ++i) {
            TreeNode* n = q.front();
            q.pop();
            if (i + 1 == level_size) out.push_back(n->value);   // the rightmost
            if (n->left)  q.push(n->left.get());
            if (n->right) q.push(n->right.get());
        }
    }
    return out;
}

// DFS alternative: visit RIGHT first, record the first node seen at each depth.
// O(h) space instead of O(w) -- better for a wide, shallow tree.
static void right_view_dfs(TreeNode* n, std::size_t depth, std::vector<int>& out) {
    if (!n) return;
    if (depth == out.size()) out.push_back(n->value);    // first node at this depth
    right_view_dfs(n->right.get(), depth + 1, out);      // RIGHT first
    right_view_dfs(n->left.get(), depth + 1, out);
}
std::vector<int> right_side_view_dfs(TreeNode* root) {
    std::vector<int> out;
    right_view_dfs(root, 0, out);
    return out;
}

// Zigzag: reverse every other level.
std::vector<std::vector<int>> zigzag(TreeNode* root) {
    auto levels = level_order(root);
    for (std::size_t i = 1; i < levels.size(); i += 2) std::ranges::reverse(levels[i]);
    return levels;
}

int main() {
    auto root = build_tree({3, 9, 20, std::nullopt, std::nullopt, 15, 7});
    assert((level_order(root.get())
            == std::vector<std::vector<int>>{{3}, {9, 20}, {15, 7}}));
    assert(level_order(nullptr).empty());
    assert((level_order(build_tree({1}).get()) == std::vector<std::vector<int>>{{1}}));

    assert((right_side_view(root.get()) == std::vector<int>{3, 20, 7}));
    assert(right_side_view(root.get()) == right_side_view_dfs(root.get()));

    // A left-heavy tree: the right side view must fall back to the left nodes.
    auto lefty = build_tree({1, 2, 3, 4});
    assert((right_side_view(lefty.get()) == std::vector<int>{1, 3, 4}));
    assert(right_side_view(lefty.get()) == right_side_view_dfs(lefty.get()));

    assert((zigzag(root.get()) == std::vector<std::vector<int>>{{3}, {20, 9}, {15, 7}}));
    return 0;
}
