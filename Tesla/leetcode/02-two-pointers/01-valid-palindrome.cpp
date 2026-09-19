// PROBLEM     Is a string a palindrome, considering only alphanumeric characters and
//             ignoring case?
// EXAMPLE     "A man, a plan, a canal: Panama" -> true ;  "race a car" -> false
// APPROACH    Converge from both ends. Skip non-alphanumerics on each side, then compare
//             the lowercased characters. No allocation, no copy of the string.
// COMPLEXITY  Time O(n), Space O(1). The "filter into a new string then compare with its
//             reverse" version is O(n) time but O(n) space -- say why you rejected it.
// FOLLOW-UPS  Unicode? -> byte-wise comparison is wrong for multi-byte encodings and for
//             case folding; you need a proper Unicode library. Say this rather than
//             pretending char works. Allow one deletion? -> on mismatch, try skipping the
//             left OR the right and check the remainder is a palindrome (LeetCode 680).

#include <cassert>
#include <cctype>
#include <string_view>

static bool is_alnum(unsigned char c) { return std::isalnum(c) != 0; }
static char lower(unsigned char c) { return static_cast<char>(std::tolower(c)); }

bool is_palindrome(std::string_view s) {
    if (s.empty()) return true;
    std::size_t lo = 0, hi = s.size() - 1;          // safe: we checked for empty
    while (lo < hi) {
        while (lo < hi && !is_alnum(static_cast<unsigned char>(s[lo]))) ++lo;
        while (lo < hi && !is_alnum(static_cast<unsigned char>(s[hi]))) --hi;
        if (lower(static_cast<unsigned char>(s[lo])) !=
            lower(static_cast<unsigned char>(s[hi]))) return false;
        ++lo;
        if (hi == 0) break;                          // guard the unsigned decrement
        --hi;
    }
    return true;
}

int main() {
    assert(is_palindrome("A man, a plan, a canal: Panama"));
    assert(!is_palindrome("race a car"));
    assert(is_palindrome(""));                       // empty
    assert(is_palindrome(" "));                      // only skipped characters
    assert(is_palindrome(".,!"));
    assert(is_palindrome("a"));
    assert(is_palindrome("aa"));
    assert(!is_palindrome("ab"));
    assert(is_palindrome("0P0"));                    // digits and letters mixed
    assert(!is_palindrome("0P"));                    // '0' != 'p': a classic wrong answer
    return 0;
}
