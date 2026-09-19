// PROBLEM     Does s2 contain a permutation of s1 as a substring?
// EXAMPLE     s1 = "ab", s2 = "eidbaooo" -> true ("ba")
// APPROACH    A permutation of s1 is exactly a substring of length |s1| with the same
//             character counts. So: a FIXED-size window of length |s1| over s2, with two
//             26-entry count arrays. Slide by adding the entering character and removing
//             the leaving one, both O(1).
//             Keep a `matches` counter of how many of the 26 letters currently agree, so
//             each step is O(1) instead of an O(26) array comparison.
// COMPLEXITY  Time O(|s2|), Space O(1). The naive "sort every window" is O(n * k log k).
// FOLLOW-UPS  All starting indices, not just existence (LC 438, Find All Anagrams)? ->
//             identical loop, push `left` instead of returning early.
//             Large alphabet? -> a hash map plus the same matches counter.

#include <array>
#include <cassert>
#include <string_view>
#include <vector>

bool check_inclusion(std::string_view pattern, std::string_view text) {
    if (pattern.size() > text.size()) return false;
    if (pattern.empty()) return true;

    std::array<int, 26> need{}, window{};
    for (char c : pattern) ++need[static_cast<std::size_t>(c - 'a')];

    int matches = 0;                                  // how many letters agree exactly
    for (int i = 0; i < 26; ++i) matches += (need[i] == 0);

    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto in = static_cast<std::size_t>(text[i] - 'a');
        if (++window[in] == need[in])      ++matches;
        else if (window[in] == need[in] + 1) --matches;   // just broke a match

        if (i >= pattern.size()) {                    // evict the element leaving
            const auto out = static_cast<std::size_t>(text[i - pattern.size()] - 'a');
            if (--window[out] == need[out])      ++matches;
            else if (window[out] == need[out] - 1) --matches;
        }
        if (matches == 26) return true;
    }
    return false;
}

// All starting indices of anagrams of `pattern` in `text` (LeetCode 438).
std::vector<int> find_anagrams(std::string_view pattern, std::string_view text) {
    std::vector<int> out;
    if (pattern.empty() || pattern.size() > text.size()) return out;
    std::array<int, 26> need{}, window{};
    for (char c : pattern) ++need[static_cast<std::size_t>(c - 'a')];
    for (std::size_t i = 0; i < text.size(); ++i) {
        ++window[static_cast<std::size_t>(text[i] - 'a')];
        if (i >= pattern.size())
            --window[static_cast<std::size_t>(text[i - pattern.size()] - 'a')];
        if (i + 1 >= pattern.size() && window == need)
            out.push_back(static_cast<int>(i + 1 - pattern.size()));
    }
    return out;
}

int main() {
    assert(check_inclusion("ab", "eidbaooo"));
    assert(!check_inclusion("ab", "eidboaoo"));
    assert(check_inclusion("adc", "dcda"));
    assert(!check_inclusion("hello", "ooolleoooleh"));
    assert(check_inclusion("", "anything"));
    assert(!check_inclusion("abc", "ab"));           // pattern longer than text
    assert(check_inclusion("a", "a"));
    assert(check_inclusion("abc", "cba"));

    assert((find_anagrams("abc", "cbaebabacd") == std::vector<int>{0, 6}));
    assert((find_anagrams("ab", "abab") == std::vector<int>{0, 1, 2}));
    assert(find_anagrams("z", "abc").empty());
    return 0;
}
