// PROBLEM     (a) Is this a valid BST? (b) Find the kth smallest value in a BST.
// EXAMPLE     [5,1,4,null,null,3,6] -> NOT a BST: 3 is in the right subtree of 5 but < 5
// APPROACH    (a) The classic WRONG answer is checking only node->left->value < node->value
//             < node->right->value. That is local and misses the example above. The right
//             answer carries a (min, max) RANGE down the recursion: everything in the left
//             subtree must be < node, and everything in the right subtree > node, all the
//             way down. Equivalently: the INORDER traversal must be strictly increasing.
//             (b) kth smallest = the kth element of the inorder traversal. Do it
//             iteratively so you can stop early at O(h + k) instead of O(n).
// COMPLEXITY  (a) O(n) time, O(h) space. (b) O(h + k) time, O(h) space.
// FOLLOW-UPS  Duplicates? -> the problem must define whether they go left or right; say
//             so rather than guessing. Use long long bounds or std::optional, because
//             INT_MIN/INT_MAX as sentinels break when a node holds those values -- that is
//             a real edge case and the reason for the optional-based version here.
//             Frequent kth-smallest queries? -> augment nodes with subtree sizes for
//             O(h) per query (an order-statistic tree); that is the "how would you make
//             this production-grade" answer.

#include "tree.hpp"
#include <cassert>
#include <optional>
#include <vector>

// Range-carrying validation. optional bounds so INT_MIN/INT_MAX values are fine.
static bool valid(const TreeNode* n, std::optional<long long> lo, std::optional<long long> hi) {
    if (!n) return true;
    const long long v = n->value;
    if (lo && v <= *lo) return false;                 // strict: no duplicates allowed
    if (hi && v >= *hi) return false;
    return valid(n->left.get(), lo, v) && valid(n->right.get(), v, hi);
}
bool is_valid_bst(const TreeNode* root) { return valid(root, std::nullopt, std::nullopt); }

// The inorder formulation: equivalent, and often easier to get right.
static bool inorder_increasing(const TreeNode* n, std::optional<long long>& prev) {
    if (!n) return true;
    if (!inorder_increasing(n->left.get(), prev)) return false;
    if (prev && *prev >= n->value) return false;
    prev = n->value;
    return inorder_increasing(n->right.get(), prev);
}
bool is_valid_bst_inorder(const TreeNode* root) {
    std::optional<long long> prev;
    return inorder_increasing(root, prev);
}

// kth smallest, iterative inorder so it stops as soon as it has the answer.
std::optional<int> kth_smallest(TreeNode* root, int k) {
    std::vector<TreeNode*> stack;
    TreeNode* curr = root;
    while (curr || !stack.empty()) {
        while (curr) { stack.push_back(curr); curr = curr->left.get(); }
        curr = stack.back();
        stack.pop_back();
        if (--k == 0) return curr->value;             // early exit: O(h + k)
        curr = curr->right.get();
    }
    return std::nullopt;                               // k larger than the tree
}

int main() {
    auto good = build_tree({2, 1, 3});
    assert(is_valid_bst(good.get()) && is_valid_bst_inorder(good.get()));

    // THE case that defeats the local-comparison solution.
    auto tricky = build_tree({5, 1, 4, std::nullopt, std::nullopt, 3, 6});
    assert(!is_valid_bst(tricky.get()) && !is_valid_bst_inorder(tricky.get()));

    assert(is_valid_bst(nullptr));
    assert(is_valid_bst(build_tree({1}).get()));
    assert(!is_valid_bst(build_tree({1, 1}).get()));    // duplicates rejected (strict)

    // A deeper violation: 6 is in the LEFT subtree of 10 but greater than it.
    auto deep = build_tree({10, 5, 15, 3, 6, std::nullopt, 20});
    assert(is_valid_bst(deep.get()));
    auto broken = build_tree({10, 5, 15, 3, 12});
    assert(!is_valid_bst(broken.get()));

    auto bst = build_tree({5, 3, 7, 2, 4, 6, 8});
    assert(kth_smallest(bst.get(), 1) == 2);
    assert(kth_smallest(bst.get(), 4) == 5);
    assert(kth_smallest(bst.get(), 7) == 8);
    assert(!kth_smallest(bst.get(), 8).has_value());
    assert(!kth_smallest(nullptr, 1).has_value());
    return 0;
}
