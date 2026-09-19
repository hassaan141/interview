// PROBLEM     Find the longest palindromic SUBSTRING (contiguous).
// EXAMPLE     "babad" -> "bab" (or "aba") ;  "cbbd" -> "bb"
// APPROACH    Two solutions, both O(n^2) time, differing in space -- and that difference
//             is the point:
//             (a) EXPAND AROUND CENTRE: there are 2n-1 centres (n single characters and
//                 n-1 gaps between them). Expand outward from each while the characters
//                 match. O(n^2) time, O(1) SPACE, and it is short enough to write
//                 correctly under pressure. This is the one to lead with.
//             (b) RANGE DP: dp[i][j] = is s[i..j] a palindrome, with
//                 dp[i][j] = (s[i]==s[j]) && (j-i < 2 || dp[i+1][j-1]). Fill by increasing
//                 LENGTH so the smaller span is already known -- that fill order is the
//                 characteristic feature of a range DP. O(n^2) time AND O(n^2) space.
// COMPLEXITY  (a) O(n^2)/O(1). (b) O(n^2)/O(n^2). Manacher's algorithm is O(n)/O(n) --
//             name it, say you would look it up rather than derive it live, and move on.
// FOLLOW-UPS  Why two kinds of centre? -> even-length palindromes have no middle
//             character. Forgetting the gap centres is THE bug in this problem.
//             Count all palindromic substrings (LC 647)? -> the same expansion, counting
//             each successful expansion instead of tracking the longest.
//             Longest palindromic SUBSEQUENCE (LC 516)? -> different problem: it is the
//             LCS of s and reverse(s), O(n^2).

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

// (a) Expand around centre: O(1) space.
std::string longest_palindrome(std::string_view s) {
    if (s.empty()) return "";
    std::size_t best_start = 0, best_len = 1;

    const auto expand = [&](std::size_t lo, std::size_t hi) {
        // hi may start one past lo (even-length centres), so guard both ends.
        while (hi < s.size() && s[lo] == s[hi]) {
            if (hi - lo + 1 > best_len) { best_len = hi - lo + 1; best_start = lo; }
            if (lo == 0) break;                          // unsigned: cannot go below 0
            --lo;
            ++hi;
        }
    };
    for (std::size_t centre = 0; centre < s.size(); ++centre) {
        expand(centre, centre);                          // odd-length centres
        if (centre + 1 < s.size()) expand(centre, centre + 1);   // EVEN-length centres
    }
    return std::string{s.substr(best_start, best_len)};
}

// (b) Range DP, filled by increasing length.
std::string longest_palindrome_dp(std::string_view s) {
    const std::size_t n = s.size();
    if (n == 0) return "";
    std::vector<std::vector<bool>> dp(n, std::vector<bool>(n, false));
    std::size_t best_start = 0, best_len = 1;

    for (std::size_t len = 1; len <= n; ++len)           // by LENGTH: the smaller span
        for (std::size_t i = 0; i + len <= n; ++i) {     // is already computed
            const std::size_t j = i + len - 1;
            dp[i][j] = (s[i] == s[j]) && (len < 3 || dp[i + 1][j - 1]);
            if (dp[i][j] && len > best_len) { best_len = len; best_start = i; }
        }
    return std::string{s.substr(best_start, best_len)};
}

// Count ALL palindromic substrings: the same expansion, counting instead of maximizing.
int count_palindromic_substrings(std::string_view s) {
    int count = 0;
    const auto expand = [&](std::size_t lo, std::size_t hi) {
        while (hi < s.size() && s[lo] == s[hi]) {
            ++count;
            if (lo == 0) break;
            --lo;
            ++hi;
        }
    };
    for (std::size_t c = 0; c < s.size(); ++c) {
        expand(c, c);
        if (c + 1 < s.size()) expand(c, c + 1);
    }
    return count;
}

int main() {
    const auto check = [](auto&& fn) {
        const std::string babad = fn(std::string_view{"babad"});
        assert(babad == "bab" || babad == "aba");        // either is valid
        assert(fn(std::string_view{"cbbd"}) == "bb");    // EVEN length
        assert(fn(std::string_view{"a"}) == "a");
        assert(fn(std::string_view{""}).empty());
        assert(fn(std::string_view{"ac"}).size() == 1);  // no palindrome longer than 1
        assert(fn(std::string_view{"aaaa"}) == "aaaa");
        assert(fn(std::string_view{"racecar"}) == "racecar");
        assert(fn(std::string_view{"abacdfgdcaba"}) == "aba");
    };
    check([](std::string_view s) { return longest_palindrome(s); });
    check([](std::string_view s) { return longest_palindrome_dp(s); });

    assert(count_palindromic_substrings("abc") == 3);     // a, b, c
    assert(count_palindromic_substrings("aaa") == 6);     // a,a,a,aa,aa,aaa
    assert(count_palindromic_substrings("") == 0);
    return 0;
}
