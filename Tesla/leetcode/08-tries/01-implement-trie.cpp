// PROBLEM     Implement a trie with insert(word), search(word), startsWith(prefix).
// APPROACH    A tree where each edge is a character. Walk the word one character at a
//             time, creating nodes as needed. `search` must additionally check the
//             is_word flag at the end -- "app" being present as a PREFIX of "apple" does
//             not make "app" a word.
// COMPLEXITY  All operations O(L) in the word length, independent of how many words are
//             stored. Space O(total characters) worst case.
// FOLLOW-UPS  Why not an unordered_set<string>? -> it cannot answer startsWith in less
//             than O(n*L). If you never need prefixes, use the hash set.
//             Memory? -> a 26-pointer array is 208 bytes per node; use a hash map for a
//             sparse alphabet, or compress single-child chains into a RADIX tree, which is
//             what a routing table or a production dictionary uses.
//             Deletion? -> below: recursive, freeing nodes that become empty leaves.
//             Ownership? -> unique_ptr children; note the destructor recurses with the
//             word length, which is bounded, so unlike a linked list it is safe.

#include <array>
#include <cassert>
#include <memory>
#include <string_view>

class Trie {
    struct Node {
        std::array<std::unique_ptr<Node>, 26> children{};
        bool is_word = false;
        bool has_children() const {
            for (const auto& c : children) if (c) return true;
            return false;
        }
    };
    Node root_;

    static std::size_t index_of(char c) { return static_cast<std::size_t>(c - 'a'); }

    // Returns true if this node can now be freed.
    static bool erase_from(Node* n, std::string_view word, std::size_t depth) {
        if (depth == word.size()) {
            if (!n->is_word) return false;          // the word was not present
            n->is_word = false;
            return !n->has_children();
        }
        const std::size_t i = index_of(word[depth]);
        Node* child = n->children[i].get();
        if (!child) return false;
        if (erase_from(child, word, depth + 1)) {
            n->children[i].reset();                  // free the now-useless subtree
            return !n->is_word && !n->has_children();
        }
        return false;
    }

public:
    void insert(std::string_view word) {
        Node* n = &root_;
        for (char c : word) {
            auto& child = n->children[index_of(c)];
            if (!child) child = std::make_unique<Node>();
            n = child.get();
        }
        n->is_word = true;
    }

    const Node* walk(std::string_view s) const {
        const Node* n = &root_;
        for (char c : s) {
            n = n->children[index_of(c)].get();
            if (!n) return nullptr;
        }
        return n;
    }

    bool search(std::string_view word) const {
        const Node* n = walk(word);
        return n && n->is_word;                      // the is_word check is the point
    }
    bool starts_with(std::string_view prefix) const { return walk(prefix) != nullptr; }
    void erase(std::string_view word) { erase_from(&root_, word, 0); }
};

int main() {
    Trie t;
    t.insert("apple");
    assert(t.search("apple"));
    assert(!t.search("app"));                        // a prefix is not a word
    assert(t.starts_with("app"));
    t.insert("app");
    assert(t.search("app"));                          // now it is
    assert(t.search("apple"));                        // and the longer word survives

    assert(!t.search("banana"));
    assert(!t.starts_with("b"));
    t.insert("");                                     // the empty word
    assert(t.search(""));
    assert(t.starts_with(""));

    t.erase("app");
    assert(!t.search("app") && t.search("apple"));    // erasing a prefix keeps the word
    t.erase("apple");
    assert(!t.search("apple") && !t.starts_with("app"));
    t.erase("nonexistent");                           // must be a safe no-op
    return 0;
}
