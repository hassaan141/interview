// PROBLEM     Can `s` be segmented into a space-separated sequence of dictionary words?
// EXAMPLE     s = "leetcode", dict = ["leet","code"] -> true
// APPROACH    STATE: dp[i] = can the PREFIX OF LENGTH i be segmented?
//             RECURRENCE: dp[i] is true if there exists j < i with dp[j] true and
//             s[j..i) in the dictionary.
//             BASE: dp[0] = true (the empty prefix is trivially segmentable) -- that base
//             case is what makes the whole recurrence work, and forgetting it makes
//             everything false.
//             Optimization: only try substrings up to the longest dictionary word, which
//             turns the inner loop from O(n) into O(max_word_len).
// COMPLEXITY  Time O(n * max_word_len) with O(1) hash lookups (O(n^2 * L) naively),
//             Space O(n).
// FOLLOW-UPS  Return ALL segmentations (LC 140)? -> backtracking WITH memoization, because
//             the number of results can be exponential; the DP above is the feasibility
//             pre-check that prunes it.
//             Why is greedy (always take the longest match) wrong? -> "aaaaab" with
//             {"aaaa","aaa","b"}: greedy takes "aaaa" then fails on "ab"; the answer needs
//             "aaa"+"aa"... the general point is that a local longest match can strand the
//             remainder.
//             Very large dictionary? -> a TRIE (section 08) walks all candidate words from
//             position j in one pass instead of hashing each substring.

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

bool word_break(std::string_view s, const std::vector<std::string>& dictionary) {
    const std::unordered_set<std::string_view> dict(dictionary.begin(), dictionary.end());
    std::size_t longest = 0;
    for (const auto& w : dictionary) longest = std::max(longest, w.size());
    if (longest == 0) return s.empty();

    std::vector<bool> dp(s.size() + 1, false);
    dp[0] = true;                                        // the empty prefix: THE base case
    for (std::size_t i = 1; i <= s.size(); ++i) {
        // Only look back as far as the longest dictionary word.
        const std::size_t lowest = i > longest ? i - longest : 0;
        for (std::size_t j = i; j-- > lowest;) {
            if (!dp[j]) continue;
            if (dict.contains(s.substr(j, i - j))) { dp[i] = true; break; }
        }
    }
    return dp[s.size()];
}

int main() {
    assert(word_break("leetcode", {"leet", "code"}));
    assert(word_break("applepenapple", {"apple", "pen"}));      // a word reused
    assert(!word_break("catsandog", {"cats", "dog", "sand", "and", "cat"}));
    assert(word_break("", {"a"}));                               // empty string
    assert(!word_break("a", {}));                                // empty dictionary
    assert(word_break("", {}));
    assert(word_break("aaaaaaa", {"aaa", "aaaa"}));
    assert(!word_break("aaaaaaab", {"aaa", "aaaa"}));
    assert(word_break("cars", {"car", "ca", "rs"}));             // needs "ca"+"rs"
    // The pathological case that makes a naive backtracker exponential.
    assert(!word_break(std::string(60, 'a') + "b", {"a", "aa", "aaa", "aaaa"}));
    return 0;
}
