// PROBLEM     Smallest substring of s containing every character of t (with multiplicity).
// EXAMPLE     s = "ADOBECODEBANC", t = "ABC" -> "BANC"
// APPROACH    Variable window, "SHORTEST" template: extend right until valid, then shrink
//             from the left WHILE it stays valid, recording the answer inside that loop.
//             Validity is tracked with a `have`/`need` counter: `need` is the number of
//             distinct required characters, `have` is how many of them are currently
//             satisfied at the required multiplicity. O(1) per step, no map comparison.
// COMPLEXITY  Time O(|s| + |t|), Space O(alphabet). Each index enters and leaves once.
// FOLLOW-UPS  Return the indices instead of a copy? -> track best_start/best_len.
//             t has duplicates? -> handled: the counts carry multiplicity (test below).
//             Very large alphabet? -> unordered_map instead of array, same logic.
//             Why record inside the shrink loop? -> because we want the SMALLEST valid
//             window; recording after restoring validity gives the longest instead.

#include <array>
#include <cassert>
#include <limits>
#include <string>
#include <string_view>

std::string min_window(std::string_view s, std::string_view t) {
    if (t.empty() || s.size() < t.size()) return "";

    std::array<int, 128> need{};
    int distinct_needed = 0;
    for (char c : t)
        if (++need[static_cast<unsigned char>(c)] == 1) ++distinct_needed;

    std::array<int, 128> window{};
    int satisfied = 0;
    std::size_t left = 0, best_start = 0;
    std::size_t best_len = std::numeric_limits<std::size_t>::max();

    for (std::size_t right = 0; right < s.size(); ++right) {
        const auto c = static_cast<unsigned char>(s[right]);
        if (++window[c] == need[c]) ++satisfied;      // note: == not >=, so it fires once

        while (satisfied == distinct_needed) {        // valid: record, then shrink
            if (right - left + 1 < best_len) {
                best_len = right - left + 1;
                best_start = left;
            }
            const auto out = static_cast<unsigned char>(s[left++]);
            if (window[out]-- == need[out]) --satisfied;
        }
    }
    return best_len == std::numeric_limits<std::size_t>::max()
               ? std::string{}
               : std::string{s.substr(best_start, best_len)};
}

int main() {
    assert(min_window("ADOBECODEBANC", "ABC") == "BANC");
    assert(min_window("a", "a") == "a");
    assert(min_window("a", "aa") == "");              // multiplicity matters
    assert(min_window("aa", "aa") == "aa");
    assert(min_window("", "a") == "");
    assert(min_window("abc", "") == "");
    assert(min_window("bba", "ab") == "ba");
    assert(min_window("cabwefgewcwaefgcf", "cae") == "cwae");
    assert(min_window("abc", "d") == "");
    return 0;
}
