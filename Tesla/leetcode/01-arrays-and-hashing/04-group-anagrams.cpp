// PROBLEM     Group a list of strings so that anagrams end up in the same group.
// EXAMPLE     ["eat","tea","tan","ate","nat","bat"]
//             -> [["eat","tea","ate"],["tan","nat"],["bat"]]
// APPROACH    Every anagram class needs one CANONICAL KEY. Two choices:
//               (a) the sorted string      -> O(k log k) per word, trivially correct
//               (b) the 26-char count      -> O(k) per word, better when words are long
//             Then one pass: map[key].push_back(word).
// COMPLEXITY  (a) Time O(n * k log k), Space O(n*k).  (b) Time O(n * k), Space O(n*k).
//             n = number of words, k = max word length. State which you chose and why.
// FOLLOW-UPS  Huge input? -> the count key avoids the per-word sort; and you can hash the
//             counts into a 64-bit value to shrink the key (with a collision fallback).
//             Streaming? -> the map is the state; it grows with distinct classes.
//             Memory bound? -> store indices, not copies of the strings.

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

// (a) Sorted-string key: the version to write first -- obviously correct.
std::vector<std::vector<std::string>> group_anagrams(const std::vector<std::string>& words) {
    std::unordered_map<std::string, std::vector<std::string>> groups;
    groups.reserve(words.size());
    for (const std::string& w : words) {
        std::string key = w;
        std::ranges::sort(key);
        groups[std::move(key)].push_back(w);
    }
    std::vector<std::vector<std::string>> out;
    out.reserve(groups.size());
    for (auto& [key, bucket] : groups) out.push_back(std::move(bucket));
    return out;
}

// (b) Count key: O(k) per word instead of O(k log k). Assumes lowercase a-z.
std::vector<std::vector<std::string>> group_anagrams_counts(
    const std::vector<std::string>& words) {
    // The key is a 26-byte string of counts -- compact, hashable, no sorting.
    std::unordered_map<std::string, std::vector<std::string>> groups;
    groups.reserve(words.size());
    for (const std::string& w : words) {
        std::array<unsigned char, 26> count{};
        for (char c : w) ++count[static_cast<std::size_t>(c - 'a')];
        groups[std::string{count.begin(), count.end()}].push_back(w);
    }
    std::vector<std::vector<std::string>> out;
    out.reserve(groups.size());
    for (auto& [key, bucket] : groups) out.push_back(std::move(bucket));
    return out;
}

// Order of groups is unspecified, so compare canonically.
static std::vector<std::vector<std::string>> canonical(
    std::vector<std::vector<std::string>> g) {
    for (auto& bucket : g) std::ranges::sort(bucket);
    std::ranges::sort(g);
    return g;
}

int main() {
    const std::vector<std::string> input{"eat", "tea", "tan", "ate", "nat", "bat"};
    const std::vector<std::vector<std::string>> expected{
        {"ate", "eat", "tea"}, {"bat"}, {"nat", "tan"}};

    assert(canonical(group_anagrams(input)) == expected);
    assert(canonical(group_anagrams_counts(input)) == expected);

    assert(group_anagrams({}).empty());
    assert(canonical(group_anagrams({""})) == std::vector<std::vector<std::string>>{{""}});
    assert(group_anagrams({"a", "b", "c"}).size() == 3);
    assert(group_anagrams({"abc", "bca", "cab"}).size() == 1);
    return 0;
}
