// PROBLEM     Generate all well-formed combinations of n pairs of parentheses.
// EXAMPLE     n = 3 -> ["((()))","(()())","(())()","()(())","()()()"]
// APPROACH    Backtracking with the stack INVARIANT baked into the recursion instead of
//             generating all 2^(2n) strings and filtering:
//               - you may add '(' while open < n
//               - you may add ')' only while close < open  (this is the stack condition)
//             Because every path is valid by construction, there is no validity check and
//             no wasted work.
// COMPLEXITY  Time and space O(4^n / sqrt(n)) -- the nth Catalan number, which is the
//             number of outputs, so this is optimal. The naive generate-and-filter is
//             O(2^(2n) * n).
// FOLLOW-UPS  Just the COUNT? -> Catalan(n) = C(2n,n)/(n+1), no enumeration needed.
//             Iterative? -> an explicit stack of (prefix, open, close) states.
//             Why does `close < open` guarantee validity? -> it is exactly the invariant
//             "the stack never underflows", checked at insertion time rather than after.
//             Note the single mutable buffer + push/pop: that is the standard trick to
//             avoid building a new string per node (course section 09 -- do not copy).

#include <cassert>
#include <string>
#include <vector>

void backtrack(int open, int close, int n, std::string& current,
               std::vector<std::string>& out) {
    if (static_cast<int>(current.size()) == 2 * n) {
        out.push_back(current);                        // one copy, at the leaf only
        return;
    }
    if (open < n) {                                    // can always open while we have room
        current.push_back('(');
        backtrack(open + 1, close, n, current, out);
        current.pop_back();                            // un-choose
    }
    if (close < open) {                                // THE invariant: never underflow
        current.push_back(')');
        backtrack(open, close + 1, n, current, out);
        current.pop_back();
    }
}

std::vector<std::string> generate_parenthesis(int n) {
    std::vector<std::string> out;
    if (n <= 0) return out;
    std::string current;
    current.reserve(static_cast<std::size_t>(2 * n));
    backtrack(0, 0, n, current, out);
    return out;
}

int main() {
    assert((generate_parenthesis(1) == std::vector<std::string>{"()"}));
    assert((generate_parenthesis(2) == std::vector<std::string>{"(())", "()()"}));
    assert((generate_parenthesis(3) == std::vector<std::string>{
        "((()))", "(()())", "(())()", "()(())", "()()()"}));
    assert(generate_parenthesis(0).empty());

    // The count is the nth Catalan number: 1, 2, 5, 14, 42, ...
    assert(generate_parenthesis(4).size() == 14);
    assert(generate_parenthesis(5).size() == 42);
    return 0;
}
