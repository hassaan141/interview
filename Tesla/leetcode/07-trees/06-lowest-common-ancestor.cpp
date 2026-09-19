// PROBLEM     Lowest common ancestor of two nodes, (a) in a BST, (b) in a general tree.
// APPROACH    (a) BST: the ordering does the work. If both values are less than the
//             current node, the LCA is in the left subtree; if both greater, the right;
//             otherwise the paths diverge HERE, so this node is the LCA. O(h), no
//             recursion needed.
//             (b) General tree: postorder. Return the node itself if it is p or q, or a
//             non-null result from a child. If BOTH children return non-null, this node is
//             the split point and therefore the LCA.
// COMPLEXITY  (a) O(h) time, O(1) space iteratively. (b) O(n) time, O(h) space.
// FOLLOW-UPS  What if a node may be ABSENT? -> the (b) algorithm silently returns the
//             other node; you must do a second existence check, or return a (found_p,
//             found_q) pair. State that assumption -- LeetCode guarantees both exist.
//             With parent pointers? -> walk up from both and intersect, like finding the
//             intersection of two linked lists.
//             MANY queries? -> binary lifting or Euler tour + sparse table: O(n log n)
//             preprocessing, O(1)/O(log n) per query. That is the systems answer.

#include "tree.hpp"
#include <algorithm>
#include <cassert>

// (a) BST: iterative, O(h) time and O(1) space.
const TreeNode* lca_bst(const TreeNode* root, int p, int q) {
    const int lo = std::min(p, q), hi = std::max(p, q);
    const TreeNode* n = root;
    while (n) {
        if (hi < n->value)       n = n->left.get();     // both on the left
        else if (lo > n->value)  n = n->right.get();    // both on the right
        else                     return n;              // they split here
    }
    return nullptr;
}

// (b) General binary tree: postorder.
const TreeNode* lca_general(const TreeNode* root, const TreeNode* p, const TreeNode* q) {
    if (!root || root == p || root == q) return root;
    const TreeNode* left  = lca_general(root->left.get(), p, q);
    const TreeNode* right = lca_general(root->right.get(), p, q);
    if (left && right) return root;                     // p and q are on opposite sides
    return left ? left : right;                         // both (or neither) on one side
}

int main() {
    auto bst = build_tree({6, 2, 8, 0, 4, 7, 9, std::nullopt, std::nullopt, 3, 5});
    assert(lca_bst(bst.get(), 2, 8)->value == 6);       // split at the root
    assert(lca_bst(bst.get(), 2, 4)->value == 2);       // an ancestor IS one of the nodes
    assert(lca_bst(bst.get(), 3, 5)->value == 4);
    assert(lca_bst(bst.get(), 0, 5)->value == 2);
    assert(lca_bst(bst.get(), 7, 9)->value == 8);
    assert(lca_bst(nullptr, 1, 2) == nullptr);

    auto tree = build_tree({3, 5, 1, 6, 2, 0, 8, std::nullopt, std::nullopt, 7, 4});
    const TreeNode* five  = find_node(tree.get(), 5);
    const TreeNode* one   = find_node(tree.get(), 1);
    const TreeNode* four  = find_node(tree.get(), 4);
    const TreeNode* seven = find_node(tree.get(), 7);

    assert(lca_general(tree.get(), five, one)->value == 3);
    assert(lca_general(tree.get(), five, four)->value == 5);   // the ancestor is one of them
    assert(lca_general(tree.get(), seven, four)->value == 2);
    assert(lca_general(tree.get(), five, five)->value == 5);   // same node twice
    assert(lca_general(nullptr, five, one) == nullptr);
    return 0;
}
