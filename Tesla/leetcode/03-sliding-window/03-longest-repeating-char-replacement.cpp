// PROBLEM     You may replace at most k characters. Find the longest substring of a
//             single repeated character achievable.
// EXAMPLE     s = "AABABBA", k = 1 -> 4 ("AABA" -> "AAAA")
// APPROACH    Variable window. A window is valid when
//                 window_length - count_of_most_frequent_char <= k
//             i.e. the characters we would have to replace fit in the budget. Extend
//             right; while invalid, shrink from the left.
//             SUBTLETY: many solutions never decrease max_count when shrinking. That is
//             correct for the ANSWER because the window only ever needs to grow past the
//             best seen so far -- a stale max_count can only make the window look invalid
//             sooner, never longer. Say this explicitly; it is the interesting part.
// COMPLEXITY  Time O(n) (alphabet is a constant), Space O(alphabet).
// FOLLOW-UPS  Longest subarray of 1s after flipping k zeros (LC 1004)? -> the same
//             template with "count of zeros in window <= k".
//             Return the substring? -> remember the best window bounds.

#include <algorithm>
#include <array>
#include <cassert>
#include <string_view>

int character_replacement(std::string_view s, int k) {
    std::array<int, 26> count{};                     // uppercase A-Z assumption
    int best = 0, max_count = 0;
    std::size_t left = 0;
    for (std::size_t right = 0; right < s.size(); ++right) {
        max_count = std::max(max_count, ++count[static_cast<std::size_t>(s[right] - 'A')]);
        // Replacements needed = window size - most frequent character's count.
        while (static_cast<int>(right - left + 1) - max_count > k)
            --count[static_cast<std::size_t>(s[left++] - 'A')];
        best = std::max(best, static_cast<int>(right - left + 1));
    }
    return best;
}

int main() {
    assert(character_replacement("ABAB", 2) == 4);
    assert(character_replacement("AABABBA", 1) == 4);
    assert(character_replacement("", 1) == 0);
    assert(character_replacement("A", 0) == 1);
    assert(character_replacement("AAAA", 0) == 4);
    assert(character_replacement("ABCDE", 0) == 1);
    assert(character_replacement("ABCDE", 4) == 5);      // budget covers everything
    assert(character_replacement("AAAB", 0) == 3);
    return 0;
}
