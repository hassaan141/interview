// PROBLEM     'A'..'Z' map to "1".."26". How many ways can a digit string be decoded?
// EXAMPLE     "226" -> 3  ("BZ", "VF", "BBF")
// APPROACH    STATE: dp[i] = the number of decodings of the first i characters.
//             RECURRENCE: the last letter used either ONE digit (valid if s[i-1] != '0')
//             or TWO digits (valid if s[i-2..i-1] is in 10..26):
//                 dp[i] = (one_digit_ok ? dp[i-1] : 0) + (two_digit_ok ? dp[i-2] : 0)
//             BASE: dp[0] = 1 (one way to decode nothing), dp[1] = s[0] != '0'.
//             This is Fibonacci with validity conditions -- and ZEROS are the whole
//             difficulty: '0' is never a standalone letter and is only valid as the second
//             digit of "10" or "20".
//             SPACE: two variables.
// COMPLEXITY  Time O(n), Space O(1).
// FOLLOW-UPS  Every "0" edge case, and they will test them: "0" -> 0, "06" -> 0,
//             "100" -> 0, "10" -> 1, "101" -> 1, "110" -> 1.
//             With '*' wildcards (LC 639)? -> the same recurrence with counted cases per
//             wildcard, and a modulus because the count explodes.
//             Overflow? -> the count is exponential in n; use long long or a modulus and
//             say which.

#include <cassert>
#include <string_view>

int num_decodings(std::string_view s) {
    if (s.empty() || s[0] == '0') return 0;

    int two_back = 1;                                  // dp[i-2], starting at dp[0] = 1
    int one_back = 1;                                  // dp[i-1], starting at dp[1] = 1
    for (std::size_t i = 1; i < s.size(); ++i) {
        int current = 0;
        if (s[i] != '0') current += one_back;          // s[i] alone as 1..9
        const int pair = (s[i - 1] - '0') * 10 + (s[i] - '0');
        if (pair >= 10 && pair <= 26) current += two_back;   // s[i-1..i] as 10..26
        if (current == 0) return 0;                     // e.g. "...30..." is undecodable
        two_back = one_back;
        one_back = current;
    }
    return one_back;
}

int main() {
    assert(num_decodings("12") == 2);            // "AB", "L"
    assert(num_decodings("226") == 3);           // "BZ", "VF", "BBF"
    assert(num_decodings("06") == 0);            // a leading zero
    assert(num_decodings("0") == 0);
    assert(num_decodings("") == 0);
    assert(num_decodings("1") == 1);
    assert(num_decodings("10") == 1);            // only "J"
    assert(num_decodings("100") == 0);           // "10" then a stranded "0"
    assert(num_decodings("101") == 1);           // "10" + "1"
    assert(num_decodings("110") == 1);           // "1" + "10"
    assert(num_decodings("27") == 1);            // 27 > 26, so only "2"+"7"
    assert(num_decodings("2101") == 1);
    assert(num_decodings("1111") == 5);          // Fibonacci when everything is valid
    assert(num_decodings("30") == 0);
    return 0;
}
