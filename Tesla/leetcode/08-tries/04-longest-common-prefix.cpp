// PROBLEM     Longest common prefix of an array of strings.
// EXAMPLE     ["flower","flow","flight"] -> "fl" ;  ["dog","racecar","car"] -> ""
// APPROACH    Vertical scan: compare character i of every string; stop at the first
//             mismatch or the first string that ends. That is O(total characters) worst
//             case but exits as soon as the prefix ends, which is usually immediately.
// COMPLEXITY  Time O(S) where S is the sum of the lengths (worst case: all strings equal);
//             Space O(1).
// FOLLOW-UPS  THIS IS THE ONE WHERE YOU SAY WHY NOT TO USE A TRIE. Building a trie is
//             O(S) time and O(S) MEMORY just to then walk down the single-child chain --
//             strictly worse than the two-pointer scan for a one-shot query. A trie only
//             pays off for REPEATED prefix queries against a fixed dictionary. Volunteering
//             that shows judgment rather than pattern-matching.
//             Many queries against a fixed set? -> then yes, build the trie once.
//             Sorting trick? -> sort, then compare only the first and last strings; O(S log n)
//             but two comparisons. Cute, not better.

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

std::string longest_common_prefix(const std::vector<std::string>& words) {
    if (words.empty()) return "";
    const std::string& first = words.front();
    for (std::size_t i = 0; i < first.size(); ++i) {
        const char c = first[i];
        for (const std::string& w : words)
            if (i >= w.size() || w[i] != c)
                return first.substr(0, i);             // first divergence
    }
    return first;                                       // the first string is the prefix
}

// The sorting variant: only the lexicographic extremes can differ.
std::string longest_common_prefix_sorted(std::vector<std::string> words) {
    if (words.empty()) return "";
    std::ranges::sort(words);
    const std::string& a = words.front();
    const std::string& b = words.back();
    std::size_t i = 0;
    while (i < a.size() && i < b.size() && a[i] == b[i]) ++i;
    return a.substr(0, i);
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn(std::vector<std::string>{"flower", "flow", "flight"}) == "fl");
        assert(fn(std::vector<std::string>{"dog", "racecar", "car"}) == "");
        assert(fn(std::vector<std::string>{}) == "");
        assert(fn(std::vector<std::string>{"alone"}) == "alone");
        assert(fn(std::vector<std::string>{"", "abc"}) == "");        // empty string
        assert(fn(std::vector<std::string>{"abc", "abc"}) == "abc");  // identical
        assert(fn(std::vector<std::string>{"abc", "ab"}) == "ab");    // one is a prefix
        assert(fn(std::vector<std::string>{"a", "a", "b"}) == "");
    };
    check([](const std::vector<std::string>& w) { return longest_common_prefix(w); });
    check([](std::vector<std::string> w) { return longest_common_prefix_sorted(std::move(w)); });
    return 0;
}
