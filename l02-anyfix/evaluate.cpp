// Project identifier: 1CAEF3A0FEDD0DEC26BA9808C69D4D22A9962768

#include "evaluate.hpp"

#include <cstdint>
#include <stack>
#include <string>

std::int64_t calc(std::int64_t a, std::int64_t b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return a / b;
        default: break;  // Should never happen, but avoids compiler warning.
    };
    return 0;  // Should never be reached.
}

std::int64_t evaluate(std::string const& expression) {
    std::stack<std::int64_t> numbers;
    std::stack<char> operators;
    for (char c : expression) {
        if (isdigit(c)) {
            std::int64_t res = c - '0';
            if (!numbers.empty() && !operators.empty()) {
                res = calc(numbers.top(), res, operators.top());
                numbers.pop();
                operators.pop();
            }
            numbers.push(res);
        } else {
            if (numbers.size() >= 2) {
                std::int64_t b = numbers.top();
                numbers.pop();
                std::int64_t a = numbers.top();
                numbers.pop();
                std::int64_t res = calc(a, b, c);
                numbers.push(res);
            } else {
                operators.push(c);
            }
        }
    }
    return numbers.top();
}
