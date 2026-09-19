// PROBLEM     Serialize a binary tree to a string and deserialize it back.
// APPROACH    PREORDER with explicit null markers. Preorder is the right choice because
//             the root comes first, so deserialization can build top-down in one pass with
//             a simple cursor. The null markers are what make the structure recoverable --
//             without them, preorder alone is ambiguous (that is why "preorder + inorder"
//             is needed to rebuild a tree when there are no markers).
//             Format here: "value,value,#,#,..." -- a comma-delimited stream where '#'
//             means null.
// COMPLEXITY  Time O(n) both ways, Space O(n) for the string plus O(h) recursion.
// FOLLOW-UPS  Why not inorder? -> inorder with null markers is NOT uniquely decodable
//             for a general binary tree. Postorder works if you read it backwards.
//             A BST specifically? -> you can skip the null markers entirely: preorder
//             alone determines a BST, because the ordering tells you where each value
//             belongs. Worth saying.
//             A real wire format? -> length-prefix or a fixed-width binary encoding
//             instead of text: no parsing ambiguity, no delimiter escaping, and it is
//             O(1) to skip a subtree. See leetcode/01/08-encode-decode-strings.cpp and
//             course section 02 -- this is the same lesson.
//             Hostile input? -> the parser below returns nullptr rather than reading past
//             the end or recursing unboundedly.

#include "tree.hpp"
#include <cassert>
#include <charconv>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

static void serialize_into(const TreeNode* n, std::string& out) {
    if (!n) { out += "#,"; return; }                   // explicit null marker
    out += std::to_string(n->value);
    out += ',';
    serialize_into(n->left.get(), out);
    serialize_into(n->right.get(), out);
}

std::string serialize(const TreeNode* root) {
    std::string out;
    serialize_into(root, out);
    return out;
}

static std::unique_ptr<TreeNode> deserialize_from(std::string_view s, std::size_t& pos,
                                                  int depth_budget) {
    if (depth_budget <= 0) return nullptr;              // guard against a hostile input
    if (pos >= s.size()) return nullptr;
    const std::size_t comma = s.find(',', pos);
    if (comma == std::string_view::npos) return nullptr;

    const std::string_view token = s.substr(pos, comma - pos);
    pos = comma + 1;
    if (token == "#") return nullptr;

    int value = 0;
    const auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
    if (ec != std::errc{} || ptr != token.data() + token.size()) return nullptr;

    auto node = std::make_unique<TreeNode>(value);
    node->left  = deserialize_from(s, pos, depth_budget - 1);
    node->right = deserialize_from(s, pos, depth_budget - 1);
    return node;
}

std::unique_ptr<TreeNode> deserialize(std::string_view s) {
    std::size_t pos = 0;
    return deserialize_from(s, pos, 10000);
}

static bool same_tree(const TreeNode* a, const TreeNode* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return a->value == b->value && same_tree(a->left.get(), b->left.get())
                                && same_tree(a->right.get(), b->right.get());
}

int main() {
    auto root = build_tree({1, 2, 3, std::nullopt, std::nullopt, 4, 5});
    const std::string s = serialize(root.get());
    assert(s == "1,2,#,#,3,4,#,#,5,#,#,");
    assert(same_tree(deserialize(s).get(), root.get()));       // round trip

    // Round-trip a set of shapes.
    const std::vector<std::vector<std::optional<int>>> shapes{
        {}, {1}, {1, 2}, {1, std::nullopt, 2}, {5, 3, 7, 2, 4, 6, 8},
        {-1, -2, -3},                                          // negative values
    };
    for (const auto& shape : shapes) {
        auto t = build_tree(shape);
        assert(same_tree(deserialize(serialize(t.get())).get(), t.get()));
    }

    assert(serialize(nullptr) == "#,");
    assert(deserialize("#,") == nullptr);

    // Malformed input must not crash or read out of bounds (run this under ASan).
    assert(deserialize("") == nullptr);
    assert(deserialize("1") == nullptr);                        // no terminator
    assert(deserialize("x,#,#,") == nullptr);                   // not a number
    return 0;
}
