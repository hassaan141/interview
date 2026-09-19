// PROBLEM     A dictionary supporting addWord(word) and search(pattern), where '.' in the
//             pattern matches ANY single character.
// EXAMPLE     add "bad","dad","mad"; search("pad")=false, search("bad")=true,
//             search(".ad")=true, search("b..")=true
// APPROACH    A trie plus a DFS. On a literal character, descend one child. On '.', try
//             EVERY non-null child. That branching is the only difference from a plain
//             trie search, and it is what makes the complexity depend on the number of
//             wildcards.
// COMPLEXITY  addWord O(L). search O(L) with no wildcards; worst case O(26^w * L) where w
//             is the number of '.' -- state this, because "it's O(L)" is wrong and the
//             interviewer is checking whether you noticed.
// FOLLOW-UPS  '*' (zero or more)? -> that is real regex; you want an NFA/Thompson
//             construction, not a trie DFS.
//             Leading wildcards are the expensive case -> bucket words by LENGTH first
//             (cheap and very effective, since a pattern's length is fixed), or index
//             suffixes too.
//             Why not std::regex? -> it is enormous in both binary size and build time
//             (course section 16) and much slower than a purpose-built matcher.

#include <array>
#include <cassert>
#include <memory>
#include <string_view>

class WordDictionary {
    struct Node {
        std::array<std::unique_ptr<Node>, 26> children{};
        bool is_word = false;
    };
    Node root_;

    static bool match(const Node* n, std::string_view pattern, std::size_t i) {
        if (!n) return false;
        if (i == pattern.size()) return n->is_word;

        const char c = pattern[i];
        if (c != '.')                                     // literal: one child
            return match(n->children[static_cast<std::size_t>(c - 'a')].get(), pattern, i + 1);

        for (const auto& child : n->children)             // wildcard: try them all
            if (child && match(child.get(), pattern, i + 1)) return true;
        return false;
    }

public:
    void add_word(std::string_view word) {
        Node* n = &root_;
        for (char c : word) {
            auto& child = n->children[static_cast<std::size_t>(c - 'a')];
            if (!child) child = std::make_unique<Node>();
            n = child.get();
        }
        n->is_word = true;
    }
    bool search(std::string_view pattern) const { return match(&root_, pattern, 0); }
};

int main() {
    WordDictionary d;
    d.add_word("bad");
    d.add_word("dad");
    d.add_word("mad");

    assert(!d.search("pad"));
    assert(d.search("bad"));
    assert(d.search(".ad"));                    // leading wildcard
    assert(d.search("b.."));                    // trailing wildcards
    assert(d.search("..."));                    // all wildcards
    assert(!d.search("...."));                  // wrong length
    assert(!d.search(".."));
    assert(!d.search("ba"));
    assert(!d.search(""));                       // nothing was added for ""

    WordDictionary e;
    e.add_word("a");
    assert(e.search("a") && e.search("."));
    assert(!e.search("aa") && !e.search(".."));
    e.add_word("");
    assert(e.search(""));
    return 0;
}
