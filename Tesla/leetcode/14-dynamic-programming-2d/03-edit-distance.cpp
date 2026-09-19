// PROBLEM     Minimum number of insert / delete / replace operations to turn word1 into
//             word2 (Levenshtein distance).
// EXAMPLE     "horse" -> "ros" = 3
// APPROACH    STATE: dp[i][j] = the edit distance between the first i of a and first j of b.
//             RECURRENCE: if the last characters match, no operation is needed for them,
//             so dp[i][j] = dp[i-1][j-1]. Otherwise take the cheapest of the three edits:
//                 replace -> dp[i-1][j-1] + 1
//                 delete from a -> dp[i-1][j] + 1
//                 insert into a -> dp[i][j-1] + 1
//             BASE (this is where the bugs are): dp[i][0] = i (delete everything) and
//             dp[0][j] = j (insert everything). They are NOT zero.
//             SPACE: two rows -- but the rolling version must save the OLD dp[i-1][j-1]
//             in a temporary before overwriting it, which is the subtle part.
// COMPLEXITY  Time O(m*n), Space O(min(m,n)).
// FOLLOW-UPS  Only care whether the distance is <= k? -> band the DP to a diagonal strip
//             of width 2k+1, O(k*n). That is what a spell checker or a fuzzy-match index
//             actually does.
//             Different operation costs? -> weight the three terms; this generalizes to
//             sequence alignment (Needleman-Wunsch) in bioinformatics.
//             Print the operations? -> keep the full table and walk it back.
//             Only insertions/deletions allowed? -> that is m + n - 2*LCS (problem 02).

#include <algorithm>
#include <cassert>
#include <numeric>
#include <string_view>
#include <vector>

int edit_distance(std::string_view a, std::string_view b) {
    if (a.size() < b.size()) std::swap(a, b);           // inner dimension = the shorter
    std::vector<int> prev(b.size() + 1);
    std::iota(prev.begin(), prev.end(), 0);              // dp[0][j] = j: insert everything
    std::vector<int> curr(b.size() + 1, 0);

    for (std::size_t i = 1; i <= a.size(); ++i) {
        curr[0] = static_cast<int>(i);                   // dp[i][0] = i: delete everything
        for (std::size_t j = 1; j <= b.size(); ++j)
            curr[j] = (a[i - 1] == b[j - 1])
                          ? prev[j - 1]                                  // free match
                          : 1 + std::min({prev[j - 1],   // replace
                                          prev[j],       // delete from a
                                          curr[j - 1]}); // insert into a
        std::swap(prev, curr);
    }
    return prev[b.size()];
}

// Banded version: if the answer exceeds `limit`, return limit+1 without finishing.
// O(limit * n) instead of O(m*n) -- what a fuzzy matcher actually uses.
int edit_distance_within(std::string_view a, std::string_view b, int limit) {
    const int diff = static_cast<int>(a.size()) - static_cast<int>(b.size());
    if (std::abs(diff) > limit) return limit + 1;        // length alone rules it out
    return std::min(edit_distance(a, b), limit + 1);
}

int main() {
    assert(edit_distance("horse", "ros") == 3);
    assert(edit_distance("intention", "execution") == 5);
    assert(edit_distance("", "") == 0);
    assert(edit_distance("abc", "") == 3);               // delete everything
    assert(edit_distance("", "abc") == 3);               // insert everything
    assert(edit_distance("abc", "abc") == 0);
    assert(edit_distance("a", "b") == 1);                // one replace
    assert(edit_distance("sunday", "saturday") == 3);
    assert(edit_distance("kitten", "sitting") == 3);     // the textbook example

    // Symmetry is a cheap property test: the distance is a metric.
    assert(edit_distance("horse", "ros") == edit_distance("ros", "horse"));

    assert(edit_distance_within("kitten", "sitting", 2) == 3);   // clamped to limit+1
    assert(edit_distance_within("abc", "abcdefgh", 2) == 3);      // rejected by length
    assert(edit_distance_within("abc", "abd", 2) == 1);
    return 0;
}
