// PROBLEM     (a) Diameter: the longest path between any two nodes, measured in EDGES.
//             (b) Balanced: is every node's left/right height difference <= 1?
// APPROACH    Both are the same pattern, and it is the most important tree pattern there
//             is: a postorder recursion that RETURNS ONE VALUE UP (the height) while
//             UPDATING A SHARED BEST on the way. The path through a node has length
//             left_height + right_height; the value returned to the parent is
//             1 + max(left, right). Those are two different quantities -- conflating them
//             is the classic bug.
// COMPLEXITY  Time O(n) -- each node visited once. Space O(h).
//             The naive version (compute height inside the diameter recursion) is
//             O(n^2); be able to explain why and to fix it, because that IS the question.
// FOLLOW-UPS  Max path sum (LC 124)? -> identical shape: return
//             max(0, best single-sided sum) up, and update the global with
//             node + left + right. Clamping negatives to 0 is the extra wrinkle.
//             Longest univalue path (LC 687)? -> same again with an equality condition.

#include "tree.hpp"
#include <algorithm>
#include <cassert>
#include <limits>

// Returns height; updates `best` with the diameter through this node.
static int height_and_diameter(const TreeNode* n, int& best) {
    if (!n) return 0;
    const int left  = height_and_diameter(n->left.get(), best);
    const int right = height_and_diameter(n->right.get(), best);
    best = std::max(best, left + right);             // path THROUGH n, in edges
    return 1 + std::max(left, right);                // height returned UP
}

int diameter(const TreeNode* root) {
    int best = 0;
    height_and_diameter(root, best);
    return best;
}

// Returns height, or -1 as a sentinel meaning "already unbalanced" -- so the whole check
// is O(n) instead of O(n^2).
static int height_or_unbalanced(const TreeNode* n) {
    if (!n) return 0;
    const int left = height_or_unbalanced(n->left.get());
    if (left == -1) return -1;                        // short-circuit
    const int right = height_or_unbalanced(n->right.get());
    if (right == -1) return -1;
    if (std::abs(left - right) > 1) return -1;
    return 1 + std::max(left, right);
}
bool is_balanced(const TreeNode* root) { return height_or_unbalanced(root) != -1; }

// Max path sum: the same shape, with negative contributions clamped away.
static int max_path_down(const TreeNode* n, int& best) {
    if (!n) return 0;
    const int left  = std::max(0, max_path_down(n->left.get(), best));   // drop if negative
    const int right = std::max(0, max_path_down(n->right.get(), best));
    best = std::max(best, n->value + left + right);   // path through n (may use both sides)
    return n->value + std::max(left, right);          // to the parent: ONE side only
}
int max_path_sum(const TreeNode* root) {
    int best = std::numeric_limits<int>::min();
    max_path_down(root, best);
    return root ? best : 0;
}

int main() {
    assert(diameter(build_tree({1, 2, 3, 4, 5}).get()) == 3);     // 4-2-1-3
    assert(diameter(build_tree({1, 2}).get()) == 1);
    assert(diameter(build_tree({1}).get()) == 0);                 // a single node: 0 EDGES
    assert(diameter(nullptr) == 0);

    assert(is_balanced(build_tree({3, 9, 20, std::nullopt, std::nullopt, 15, 7}).get()));
    assert(!is_balanced(build_tree({1, 2, 2, 3, 3, std::nullopt, std::nullopt, 4, 4}).get()));
    assert(is_balanced(nullptr));
    assert(is_balanced(build_tree({1}).get()));

    assert(max_path_sum(build_tree({1, 2, 3}).get()) == 6);
    assert(max_path_sum(build_tree({-10, 9, 20, std::nullopt, std::nullopt, 15, 7}).get()) == 42);
    assert(max_path_sum(build_tree({-3}).get()) == -3);           // all negative
    assert(max_path_sum(build_tree({2, -1}).get()) == 2);
    return 0;
}
