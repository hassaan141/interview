// PROBLEM     Maximum depth of a binary tree. Then: write all four traversals, both
//             recursively and iteratively.
// APPROACH    Depth is the canonical POSTORDER computation: get the children's answers,
//             combine, return up. 1 + max(left, right).
//             The iterative versions matter because recursion depth == tree height, and a
//             degenerate tree overflows the stack with no diagnostic (course section 03).
// COMPLEXITY  Time O(n) for all of them. Space O(h) for DFS (h = height, n worst case),
//             O(w) for BFS (w = max width, n/2 worst case). State which you are optimizing.
// FOLLOW-UPS  MINIMUM depth (LC 111)? -> BFS is better: it stops at the first leaf
//             instead of exploring the whole tree. That asymmetry is the point of the
//             question. Also: min depth must ignore a null child of a one-child node.
//             N-ary tree? -> same shape, loop over children.

#include "tree.hpp"
#include <algorithm>
#include <cassert>
#include <vector>

int max_depth(const TreeNode* root) {                 // postorder: combine children
    if (!root) return 0;
    return 1 + std::max(max_depth(root->left.get()), max_depth(root->right.get()));
}

int max_depth_iterative(TreeNode* root) {             // BFS: count levels
    if (!root) return 0;
    std::queue<TreeNode*> q;
    q.push(root);
    int depth = 0;
    while (!q.empty()) {
        const std::size_t level_size = q.size();      // SNAPSHOT before the inner loop
        for (std::size_t i = 0; i < level_size; ++i) {
            TreeNode* n = q.front();
            q.pop();
            if (n->left)  q.push(n->left.get());
            if (n->right) q.push(n->right.get());
        }
        ++depth;
    }
    return depth;
}

int min_depth(TreeNode* root) {                        // BFS stops at the FIRST leaf
    if (!root) return 0;
    std::queue<TreeNode*> q;
    q.push(root);
    int depth = 1;
    while (!q.empty()) {
        const std::size_t level_size = q.size();
        for (std::size_t i = 0; i < level_size; ++i) {
            TreeNode* n = q.front();
            q.pop();
            if (!n->left && !n->right) return depth;   // first leaf: done
            if (n->left)  q.push(n->left.get());
            if (n->right) q.push(n->right.get());
        }
        ++depth;
    }
    return depth;
}

// --- the four traversals, recursive and iterative -------------------------
void preorder(const TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    out.push_back(n->value);                           // visit BEFORE descending
    preorder(n->left.get(), out);
    preorder(n->right.get(), out);
}
void inorder(const TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    inorder(n->left.get(), out);
    out.push_back(n->value);                           // visit BETWEEN -> sorted for a BST
    inorder(n->right.get(), out);
}
void postorder(const TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    postorder(n->left.get(), out);
    postorder(n->right.get(), out);
    out.push_back(n->value);                           // visit AFTER -> children first
}

std::vector<int> preorder_iterative(TreeNode* root) {
    std::vector<int> out;
    if (!root) return out;
    std::vector<TreeNode*> stack{root};
    while (!stack.empty()) {
        TreeNode* n = stack.back();
        stack.pop_back();
        out.push_back(n->value);
        if (n->right) stack.push_back(n->right.get());  // right first: LIFO
        if (n->left)  stack.push_back(n->left.get());
    }
    return out;
}

std::vector<int> inorder_iterative(TreeNode* root) {
    std::vector<int> out;
    std::vector<TreeNode*> stack;
    TreeNode* curr = root;
    while (curr || !stack.empty()) {
        while (curr) { stack.push_back(curr); curr = curr->left.get(); }  // go left
        curr = stack.back();
        stack.pop_back();
        out.push_back(curr->value);
        curr = curr->right.get();
    }
    return out;
}

int main() {
    auto root = build_tree({3, 9, 20, std::nullopt, std::nullopt, 15, 7});
    assert(max_depth(root.get()) == 3);
    assert(max_depth_iterative(root.get()) == 3);
    assert(min_depth(root.get()) == 2);                // node 9 is a leaf at depth 2

    assert(max_depth(nullptr) == 0);
    assert(min_depth(nullptr) == 0);
    auto single = build_tree({1});
    assert(max_depth(single.get()) == 1 && min_depth(single.get()) == 1);

    // A degenerate (linked-list shaped) tree.
    auto skew = build_tree({1, 2, std::nullopt, 3});
    assert(max_depth(skew.get()) == 3);
    assert(min_depth(skew.get()) == 3);                // no early leaf

    std::vector<int> pre, in, post;
    preorder(root.get(), pre);
    inorder(root.get(), in);
    postorder(root.get(), post);
    assert((pre  == std::vector<int>{3, 9, 20, 15, 7}));
    assert((in   == std::vector<int>{9, 3, 15, 20, 7}));
    assert((post == std::vector<int>{9, 15, 7, 20, 3}));

    assert(preorder_iterative(root.get()) == pre);     // iterative matches recursive
    assert(inorder_iterative(root.get()) == in);
    return 0;
}
