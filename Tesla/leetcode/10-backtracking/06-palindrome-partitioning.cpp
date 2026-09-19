// PROBLEM     Partition a string into substrings that are ALL palindromes; return every
//             such partition.
// EXAMPLE     "aab" -> [["a","a","b"],["aa","b"]]
// APPROACH    Backtracking over CUT POINTS: from position `start`, try every end position,
//             and if s[start..end] is a palindrome, take it and recurse from end+1.
//             The optimization that matters: precompute an O(n^2) DP table
//             is_pal[i][j] so each palindrome check is O(1) instead of O(n). Without it the
//             total is O(n * 2^n); with it, O(2^n) plus the O(n^2) setup.
//             Recurrence: is_pal[i][j] = (s[i]==s[j]) && (j-i < 2 || is_pal[i+1][j-1]),
//             filled by increasing length so the smaller span is already known.
// COMPLEXITY  O(n * 2^n) output size (2^(n-1) partitions worst case, each O(n) to copy),
//             O(n^2) extra space for the table.
// FOLLOW-UPS  MINIMUM number of cuts (LC 132)? -> do not enumerate; that is a 1D DP over
//             the same table, O(n^2). Knowing when to stop enumerating is the point.
//             Longest palindromic substring (LC 5)? -> the same table, or expand-around
//             centre in O(n^2)/O(1), or Manacher in O(n).

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

namespace {
void partition_dfs(std::string_view s, std::size_t start,
                   const std::vector<std::vector<bool>>& is_pal,
                   std::vector<std::string>& current,
                   std::vector<std::vector<std::string>>& out) {
    if (start == s.size()) { out.push_back(current); return; }
    for (std::size_t end = start; end < s.size(); ++end) {
        if (!is_pal[start][end]) continue;                  // O(1) thanks to the table
        current.emplace_back(s.substr(start, end - start + 1));
        partition_dfs(s, end + 1, is_pal, current, out);
        current.pop_back();                                  // un-choose
    }
}
}  // namespace

std::vector<std::vector<std::string>> palindrome_partition(std::string_view s) {
    std::vector<std::vector<std::string>> out;
    if (s.empty()) return out;
    const std::size_t n = s.size();

    // is_pal[i][j] for the substring s[i..j], filled by increasing length.
    std::vector<std::vector<bool>> is_pal(n, std::vector<bool>(n, false));
    for (std::size_t len = 1; len <= n; ++len)
        for (std::size_t i = 0; i + len <= n; ++i) {
            const std::size_t j = i + len - 1;
            is_pal[i][j] = (s[i] == s[j]) && (len < 3 || is_pal[i + 1][j - 1]);
        }

    std::vector<std::string> current;
    partition_dfs(s, 0, is_pal, current, out);
    return out;
}

// Minimum cuts: the same table, but a DP instead of an enumeration. O(n^2).
int min_cut(std::string_view s) {
    const std::size_t n = s.size();
    if (n < 2) return 0;
    std::vector<std::vector<bool>> is_pal(n, std::vector<bool>(n, false));
    for (std::size_t len = 1; len <= n; ++len)
        for (std::size_t i = 0; i + len <= n; ++i) {
            const std::size_t j = i + len - 1;
            is_pal[i][j] = (s[i] == s[j]) && (len < 3 || is_pal[i + 1][j - 1]);
        }
    // cuts[i] = minimum cuts needed for the prefix of length i.
    std::vector<int> cuts(n + 1, 0);
    for (std::size_t i = 1; i <= n; ++i) {
        cuts[i] = static_cast<int>(i) - 1;                   // worst case: cut everywhere
        for (std::size_t j = 0; j < i; ++j)
            if (is_pal[j][i - 1]) cuts[i] = std::min(cuts[i], j == 0 ? 0 : cuts[j] + 1);
    }
    return cuts[n];
}

int main() {
    using V = std::vector<std::vector<std::string>>;
    assert((palindrome_partition("aab") == V{{"a", "a", "b"}, {"aa", "b"}}));
    assert((palindrome_partition("a") == V{{"a"}}));
    assert(palindrome_partition("").empty());
    assert(palindrome_partition("ab").size() == 1);           // only ["a","b"]
    assert(palindrome_partition("aaa").size() == 4);          // a|a|a, a|aa, aa|a, aaa
    assert(palindrome_partition("racecar").size() == 4);  // racecar, r|aceca|r, ... 

    assert(min_cut("aab") == 1);
    assert(min_cut("a") == 0);
    assert(min_cut("ab") == 1);
    assert(min_cut("aaa") == 0);
    assert(min_cut("abcde") == 4);
    assert(min_cut("racecar") == 0);
    return 0;
}
