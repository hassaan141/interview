// Shared tree helpers for this section. A tree that OWNS its children via unique_ptr,
// plus raw observer pointers for the algorithms (the LeetCode signature).
//
// Ownership note worth saying in an interview: unique_ptr children means no leaks and no
// manual delete, but the destructor recurses once per level -- fine for a balanced tree,
// a stack overflow for a degenerate one. An arena (a vector of nodes + indices) avoids
// both and is what a real system uses.
#pragma once
#include <memory>
#include <optional>
#include <queue>
#include <vector>

struct TreeNode {
    int value{};
    std::unique_ptr<TreeNode> left;
    std::unique_ptr<TreeNode> right;
    explicit TreeNode(int v) : value{v} {}
};

// Build from a level-order list with std::nullopt for missing children, LeetCode style.
inline std::unique_ptr<TreeNode> build_tree(const std::vector<std::optional<int>>& level) {
    if (level.empty() || !level[0].has_value()) return nullptr;
    auto root = std::make_unique<TreeNode>(*level[0]);
    std::queue<TreeNode*> q;
    q.push(root.get());
    std::size_t i = 1;
    while (!q.empty() && i < level.size()) {
        TreeNode* node = q.front();
        q.pop();
        if (i < level.size()) {
            if (level[i].has_value()) {
                node->left = std::make_unique<TreeNode>(*level[i]);
                q.push(node->left.get());
            }
            ++i;
        }
        if (i < level.size()) {
            if (level[i].has_value()) {
                node->right = std::make_unique<TreeNode>(*level[i]);
                q.push(node->right.get());
            }
            ++i;
        }
    }
    return root;
}

inline TreeNode* find_node(TreeNode* root, int value) {
    if (!root) return nullptr;
    if (root->value == value) return root;
    if (TreeNode* l = find_node(root->left.get(), value)) return l;
    return find_node(root->right.get(), value);
}
