// PROBLEM     Length of the longest substring with no repeated characters.
// EXAMPLE     "abcabcbb" -> 3 ("abc") ;  "bbbbb" -> 1 ;  "pwwkew" -> 3 ("wke")
// APPROACH    Variable window, "longest" template. Extend right; while the new character
//             is already in the window, shrink from the left. Record the length AFTER
//             restoring validity.
//             Optimization: instead of shrinking one step at a time, jump `left` straight
//             past the previous occurrence -- still O(n), fewer iterations.
// COMPLEXITY  Time O(n) (each index visited at most twice), Space O(min(n, alphabet)).
// FOLLOW-UPS  Return the substring, not the length? -> remember the best window's start.
//             At most k distinct characters (LC 340)? -> same skeleton, the condition
//             becomes distinct_count > k. Unicode? -> the 128-array assumption breaks.

#include <algorithm>
#include <array>
#include <cassert>
#include <string_view>

// Jump version: left moves directly past the previous occurrence.
int length_of_longest_substring(std::string_view s) {
    std::array<int, 128> last_index;                 // ASCII assumption -- state it
    last_index.fill(-1);
    int best = 0, left = 0;
    for (int right = 0; right < static_cast<int>(s.size()); ++right) {
        const auto c = static_cast<unsigned char>(s[right]);
        // Only jump FORWARD: a stale occurrence before `left` must not move it back.
        left = std::max(left, last_index[c] + 1);
        last_index[c] = right;
        best = std::max(best, right - left + 1);
    }
    return best;
}

// The explicit shrink version -- the template to reach for under pressure.
int length_of_longest_substring_shrink(std::string_view s) {
    std::array<int, 128> in_window{};
    int best = 0;
    std::size_t left = 0;
    for (std::size_t right = 0; right < s.size(); ++right) {
        const auto c = static_cast<unsigned char>(s[right]);
        ++in_window[c];
        while (in_window[c] > 1)                      // shrink until valid
            --in_window[static_cast<unsigned char>(s[left++])];
        best = std::max(best, static_cast<int>(right - left + 1));
    }
    return best;
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn("abcabcbb") == 3);
        assert(fn("bbbbb") == 1);
        assert(fn("pwwkew") == 3);
        assert(fn("") == 0);
        assert(fn(" ") == 1);
        assert(fn("au") == 2);
        assert(fn("dvdf") == 3);       // the case the naive `left = last+1` gets wrong
        assert(fn("abba") == 2);       // the case that needs the max() guard
    };
    check([](std::string_view s) { return length_of_longest_substring(s); });
    check([](std::string_view s) { return length_of_longest_substring_shrink(s); });
    return 0;
}
