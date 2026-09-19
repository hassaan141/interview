// PROBLEM     Invert a binary tree (mirror it).
// EXAMPLE     [4,2,7,1,3,6,9] -> [4,7,2,9,6,3,1]
// APPROACH    Swap every node's children. Any traversal works, because the swap at each
//             node is independent of the others. With unique_ptr children the swap is
//             just std::swap of the two pointers -- no allocation, no copying.
// COMPLEXITY  Time O(n), Space O(h) recursive / O(w) iterative BFS.
// FOLLOW-UPS  Iteratively? -> BFS or an explicit stack, below.
//             Symmetric tree (LC 101)? -> do NOT invert and compare; recurse on the two
//             subtrees in mirrored order. Inverting mutates the input, which is usually
//             the wrong thing to do to answer a question.
//             This is the problem Max Howell was famously rejected over; the real point
//             is whether you can write clean recursion and discuss the iterative form.

#include "tree.hpp"
#include <algorithm>
#include <cassert>
#include <vector>

void invert(TreeNode* root) {
    if (!root) return;
    std::swap(root->left, root->right);               // O(1): unique_ptr pointer swap
    invert(root->left.get());
    invert(root->right.get());
}

void invert_iterative(TreeNode* root) {
    if (!root) return;
    std::vector<TreeNode*> stack{root};
    while (!stack.empty()) {
        TreeNode* n = stack.back();
        stack.pop_back();
        std::swap(n->left, n->right);
        if (n->left)  stack.push_back(n->left.get());
        if (n->right) stack.push_back(n->right.get());
    }
}

// Symmetric: compare the two subtrees in MIRRORED order, without mutating anything.
bool is_mirror(const TreeNode* a, const TreeNode* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return a->value == b->value &&
           is_mirror(a->left.get(), b->right.get()) &&
           is_mirror(a->right.get(), b->left.get());
}
bool is_symmetric(const TreeNode* root) {
    return !root || is_mirror(root->left.get(), root->right.get());
}

static std::vector<int> level_values(TreeNode* root) {
    std::vector<int> out;
    if (!root) return out;
    std::queue<TreeNode*> q;
    q.push(root);
    while (!q.empty()) {
        TreeNode* n = q.front();
        q.pop();
        out.push_back(n->value);
        if (n->left)  q.push(n->left.get());
        if (n->right) q.push(n->right.get());
    }
    return out;
}

int main() {
    auto root = build_tree({4, 2, 7, 1, 3, 6, 9});
    invert(root.get());
    assert((level_values(root.get()) == std::vector<int>{4, 7, 2, 9, 6, 3, 1}));
    invert(root.get());                                 // inverting twice is the identity
    assert((level_values(root.get()) == std::vector<int>{4, 2, 7, 1, 3, 6, 9}));

    invert_iterative(root.get());
    assert((level_values(root.get()) == std::vector<int>{4, 7, 2, 9, 6, 3, 1}));

    invert(nullptr);                                    // must not crash
    auto one = build_tree({1});
    invert(one.get());
    assert((level_values(one.get()) == std::vector<int>{1}));

    assert(is_symmetric(build_tree({1, 2, 2, 3, 4, 4, 3}).get()));
    assert(!is_symmetric(build_tree({1, 2, 2, std::nullopt, 3, std::nullopt, 3}).get()));
    assert(is_symmetric(nullptr));
    return 0;
}
