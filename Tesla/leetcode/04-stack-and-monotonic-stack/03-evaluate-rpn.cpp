// PROBLEM     Evaluate an expression in Reverse Polish Notation.
// EXAMPLE     ["2","1","+","3","*"] -> 9   ((2+1)*3)
// APPROACH    Push numbers; on an operator pop two operands (ORDER MATTERS: the second
//             pop is the left-hand side) and push the result. RPN needs no parentheses
//             and no precedence rules, which is exactly why stack machines and compilers'
//             back ends use it.
// COMPLEXITY  Time O(n), Space O(n).
// FOLLOW-UPS  Infix input? -> shunting-yard to convert, then this. Division semantics? ->
//             C++ truncates toward zero, so -7/2 == -3, which matches LeetCode; Python
//             floors, which does NOT -- a real portability trap.
//             Malformed input? -> this returns nullopt rather than reading an empty stack;
//             a parser must never trust its input.
//             Overflow? -> intermediate results can exceed int; use long long.

#include <cassert>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

std::optional<long long> eval_rpn(const std::vector<std::string>& tokens) {
    std::vector<long long> stack;
    stack.reserve(tokens.size() / 2 + 1);

    for (const std::string& tok : tokens) {
        const bool is_op = tok.size() == 1 && (tok == "+" || tok == "-" || tok == "*" || tok == "/");
        if (!is_op) {
            long long value = 0;
            const auto [ptr, ec] = std::from_chars(tok.data(), tok.data() + tok.size(), value);
            if (ec != std::errc{} || ptr != tok.data() + tok.size()) return std::nullopt;
            stack.push_back(value);
            continue;
        }
        if (stack.size() < 2) return std::nullopt;     // malformed: too few operands
        const long long rhs = stack.back(); stack.pop_back();
        const long long lhs = stack.back(); stack.pop_back();   // SECOND pop is the left
        switch (tok[0]) {
            case '+': stack.push_back(lhs + rhs); break;
            case '-': stack.push_back(lhs - rhs); break;
            case '*': stack.push_back(lhs * rhs); break;
            case '/':
                if (rhs == 0) return std::nullopt;     // not UB on our watch
                stack.push_back(lhs / rhs);            // truncates toward zero in C++
                break;
            default: return std::nullopt;
        }
    }
    if (stack.size() != 1) return std::nullopt;        // malformed: leftover operands
    return stack.back();
}

int main() {
    assert(eval_rpn({"2", "1", "+", "3", "*"}) == 9);
    assert(eval_rpn({"4", "13", "5", "/", "+"}) == 6);              // 4 + (13/5) = 4+2
    assert(eval_rpn({"10", "6", "9", "3", "+", "-11", "*", "/", "*", "17", "+", "5", "+"}) == 22);
    assert(eval_rpn({"5"}) == 5);
    assert(eval_rpn({"-7", "2", "/"}) == -3);          // truncation toward zero
    assert(!eval_rpn({}).has_value());                  // empty
    assert(!eval_rpn({"+"}).has_value());               // too few operands
    assert(!eval_rpn({"1", "2"}).has_value());          // leftover operands
    assert(!eval_rpn({"1", "0", "/"}).has_value());     // division by zero
    assert(!eval_rpn({"x", "1", "+"}).has_value());     // not a number
    return 0;
}
