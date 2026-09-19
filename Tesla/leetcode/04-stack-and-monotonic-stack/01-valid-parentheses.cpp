// PROBLEM     Is a string of ()[]{} correctly matched and nested?
// EXAMPLE     "()[]{}" -> true ;  "([)]" -> false ;  "(" -> false
// APPROACH    Push openers; on a closer, the top must be its match. At the end the stack
//             must be empty (otherwise there are unclosed openers). An odd-length string
//             can be rejected immediately.
// COMPLEXITY  Time O(n), Space O(n) worst case (all openers).
// FOLLOW-UPS  Minimum insertions to balance (LC 921)? -> count unmatched openers and
//             closers. Longest valid substring (LC 32)? -> a stack of indices, or a DP.
//             Generalized tags (HTML)? -> the stack holds names, not single characters.
//             Why is the "count" solution wrong? -> for a single bracket type a counter
//             suffices, but with multiple types you need the ORDER, which is the stack.

#include <cassert>
#include <string_view>
#include <vector>

bool is_valid(std::string_view s) {
    if (s.size() % 2 != 0) return false;             // cheap early rejection
    std::vector<char> stack;                          // vector, not std::stack
    stack.reserve(s.size() / 2);

    for (char c : s) {
        switch (c) {
            case '(': case '[': case '{':
                stack.push_back(c);
                break;
            case ')': case ']': case '}': {
                if (stack.empty()) return false;      // a closer with nothing open
                const char open = stack.back();
                stack.pop_back();
                const bool matched = (c == ')' && open == '(') ||
                                     (c == ']' && open == '[') ||
                                     (c == '}' && open == '{');
                if (!matched) return false;
                break;
            }
            default:
                return false;                         // unexpected character
        }
    }
    return stack.empty();                             // leftovers mean unclosed
}

int main() {
    assert(is_valid("()"));
    assert(is_valid("()[]{}"));
    assert(is_valid("{[()]}"));
    assert(!is_valid("(]"));
    assert(!is_valid("([)]"));                        // matched counts, wrong nesting
    assert(!is_valid("("));
    assert(!is_valid(")"));
    assert(!is_valid("((("));
    assert(is_valid(""));
    assert(!is_valid("(("));
    assert(!is_valid("a"));                           // unexpected character
    return 0;
}
