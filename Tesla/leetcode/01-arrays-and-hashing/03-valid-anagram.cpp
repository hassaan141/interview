// PROBLEM     Are two strings anagrams of each other?
// EXAMPLE     "anagram", "nagaram" -> true ;  "rat", "car" -> false
// APPROACH    Equal length is a necessary condition, so check it first and return early.
//             Then count characters: one array pass adds for s and subtracts for t, and
//             every count must end at zero. A fixed 26-entry array beats a hash map --
//             no hashing, no allocation, and it lives in L1.
// COMPLEXITY  Time O(n), Space O(1) -- the counter array is a fixed 26 ints regardless of
//             n. Sorting both strings is O(n log n) time and O(1) extra space if you may
//             mutate; mention it as the one-liner.
// FOLLOW-UPS  Unicode? -> the 26-array assumption breaks; use unordered_map<char32_t,int>
//             and say so explicitly, because "assume lowercase ASCII" is a real
//             requirements question, not a detail. Case/whitespace insensitive? ->
//             normalize first. Many strings against one? -> precompute the counts once.

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <string_view>
#include <unordered_map>

// Assumes lowercase a-z. STATE THIS ASSUMPTION OUT LOUD in an interview.
bool is_anagram(std::string_view s, std::string_view t) {
    if (s.size() != t.size()) return false;         // cheap necessary condition

    std::array<int, 26> count{};
    for (std::size_t i = 0; i < s.size(); ++i) {
        ++count[static_cast<std::size_t>(s[i] - 'a')];
        --count[static_cast<std::size_t>(t[i] - 'a')];
    }
    return std::ranges::all_of(count, [](int c) { return c == 0; });
}

// No alphabet assumption: works for any char, at the cost of hashing and allocation.
bool is_anagram_general(std::string_view s, std::string_view t) {
    if (s.size() != t.size()) return false;
    std::unordered_map<char, int> count;
    count.reserve(s.size());
    for (char c : s) ++count[c];
    for (char c : t) {
        auto it = count.find(c);
        if (it == count.end() || --it->second < 0) return false;
    }
    return true;                                    // sizes match, so no leftovers
}

int main() {
    assert(is_anagram("anagram", "nagaram"));
    assert(!is_anagram("rat", "car"));
    assert(is_anagram("", ""));
    assert(!is_anagram("a", "ab"));                 // length check
    assert(is_anagram("aabb", "bbaa"));
    assert(!is_anagram("aabb", "abbb"));            // same length, different counts

    assert(is_anagram_general("Listen", "Silent") == false);   // case matters here
    assert(is_anagram_general("listen", "silent"));
    assert(is_anagram_general("a!b", "b!a"));       // non-alphabetic characters
    return 0;
}
