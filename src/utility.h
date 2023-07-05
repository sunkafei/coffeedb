#ifndef UTILITY_GUARD
#define UTILITY_GUARD
#include <iostream>
#include <string>
auto output(const auto& param) {
    if constexpr (requires {param.first; param.second; }) {
        std::cerr << '(' << param.first << ' ' << param.second << ')';
    }
    else if constexpr (requires {param.push_back; }) {
        std::cerr << '[';
        for (const auto &val : param) {
            output(val);
        }
        std::cerr << ']';
    }
    else if constexpr (requires {param.count; }) {
        std::cerr << '{';
        for (const auto &val : param) {
            output(val);
        }
        std::cerr << '}';
    }
    else {
        std::cerr << param;
    }
}; 
auto print(const auto&... params) {
    std::cerr << "\033[1;31m";
    (... , (output(params), std::cerr << ' '));
    std::cerr << "\033[0m" << std::endl; 
}
#endif