// PROBLEM     Length of the longest common subsequence of two strings (not contiguous).
// EXAMPLE     "abcde", "ace" -> 3 ("ace")
// APPROACH    STATE: dp[i][j] = the LCS of the first i characters of a and the first j of
//             b. Indexing by LENGTH (not position) makes row 0 and column 0 the natural
//             empty base cases, all zero -- that convention removes most off-by-ones, and
//             it is why the comparison reads a[i-1] vs b[j-1].
//             RECURRENCE: if the last characters match, they can both be used, so
//             dp[i][j] = dp[i-1][j-1] + 1; otherwise drop one character from one string
//             and take the better: max(dp[i-1][j], dp[i][j-1]).
//             SPACE: only the previous row is read, so TWO rows suffice -- O(m*n) -> O(n).
// COMPLEXITY  Time O(m*n), Space O(min(m,n)) after the rolling optimization.
// FOLLOW-UPS  Reconstruct the subsequence? -> that needs the FULL table to walk back
//             through, so the space optimization and reconstruction are mutually
//             exclusive (Hirschberg's algorithm gets both in O(min(m,n)) space and O(m*n)
//             time -- worth naming).
//             Longest common SUBSTRING (contiguous)? -> different recurrence: on a
//             mismatch reset to 0 rather than taking a max, and track the global best.
//             LCS of 3 strings? -> a 3D table, O(n^3).
//             This is the core of `diff`, and edit distance (problem 03) is its sibling.

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

// O(min(m,n)) space with two rolling rows.
int longest_common_subsequence(std::string_view a, std::string_view b) {
    if (a.size() < b.size()) std::swap(a, b);           // keep the inner dimension small
    std::vector<int> prev(b.size() + 1, 0), curr(b.size() + 1, 0);
    for (std::size_t i = 1; i <= a.size(); ++i) {
        for (std::size_t j = 1; j <= b.size(); ++j)
            curr[j] = (a[i - 1] == b[j - 1]) ? prev[j - 1] + 1
                                             : std::max(prev[j], curr[j - 1]);
        std::swap(prev, curr);
    }
    return prev[b.size()];
}

// Full table version, so we can reconstruct the actual subsequence.
std::string lcs_string(std::string_view a, std::string_view b) {
    std::vector<std::vector<int>> dp(a.size() + 1, std::vector<int>(b.size() + 1, 0));
    for (std::size_t i = 1; i <= a.size(); ++i)
        for (std::size_t j = 1; j <= b.size(); ++j)
            dp[i][j] = (a[i - 1] == b[j - 1]) ? dp[i - 1][j - 1] + 1
                                              : std::max(dp[i - 1][j], dp[i][j - 1]);
    std::string out;
    std::size_t i = a.size(), j = b.size();
    while (i > 0 && j > 0) {                             // walk the table backwards
        if (a[i - 1] == b[j - 1]) { out += a[i - 1]; --i; --j; }
        else if (dp[i - 1][j] >= dp[i][j - 1]) --i;
        else --j;
    }
    std::ranges::reverse(out);
    return out;
}

// Longest common SUBSTRING (contiguous): a different recurrence.
int longest_common_substring(std::string_view a, std::string_view b) {
    std::vector<int> prev(b.size() + 1, 0), curr(b.size() + 1, 0);
    int best = 0;
    for (std::size_t i = 1; i <= a.size(); ++i) {
        for (std::size_t j = 1; j <= b.size(); ++j) {
            curr[j] = (a[i - 1] == b[j - 1]) ? prev[j - 1] + 1 : 0;   // RESET on mismatch
            best = std::max(best, curr[j]);
        }
        std::swap(prev, curr);
    }
    return best;
}

int main() {
    assert(longest_common_subsequence("abcde", "ace") == 3);
    assert(longest_common_subsequence("abc", "abc") == 3);
    assert(longest_common_subsequence("abc", "def") == 0);
    assert(longest_common_subsequence("", "abc") == 0);
    assert(longest_common_subsequence("", "") == 0);
    assert(longest_common_subsequence("a", "a") == 1);
    assert(longest_common_subsequence("bsbininm", "jmjkbkjkv") == 1);

    assert(lcs_string("abcde", "ace") == "ace");
    assert(lcs_string("abc", "def").empty());
    assert(lcs_string("AGGTAB", "GXTXAYB") == "GTAB");

    assert(longest_common_substring("abcde", "ace") == 1);        // NOT 3
    assert(longest_common_substring("abcdef", "zabcy") == 3);     // "abc"
    assert(longest_common_substring("abc", "") == 0);
    return 0;
}
