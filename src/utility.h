#ifndef UTILITY_GUARD
#define UTILITY_GUARD
#include <iostream>
#include <string>
#include <utility>
#include <stdexcept>
#include <charconv>
#include <regex>
auto output(auto&& param) {
    if constexpr (requires {std::cerr << param; }) {
        std::cerr << param;
    }
    else if constexpr (requires {param.first; param.second; }) {
        std::cerr << '(' << param.first << ' ' << param.second << ')';
    }
    else if constexpr (requires {param.push_back(param[0]); }) {
        std::cerr << '[';
        for (auto iter = param.begin(); iter != param.end(); ++iter) {
            if (iter != param.begin())
                std::cerr << ' ';
            output(*iter);
        }
        std::cerr << ']';
    }
    else if constexpr (requires {param.count(*param.begin()); }) {
        std::cerr << '{';
        for (auto iter = param.begin(); iter != param.end(); ++iter) {
            if (iter != param.begin())
                std::cerr << ' ';
            output(*iter);
        }
        std::cerr << '}';
    }
    else {
        throw std::logic_error("Output error");
    }
}
auto print(auto&&... params) {
    std::cerr << "\033[1;32m";
    (... , (output(std::forward<decltype(params)>(params)), std::cerr << ' '));
    std::cerr << "\033[0m" << std::endl; 
}
auto error(auto&&... params) {
    std::cerr << "\033[1;31m";
    (... , (output(std::forward<decltype(params)>(params)), std::cerr << ' '));
    std::cerr << "\033[0m" << std::endl; 
}
auto value_conv(std::string str, auto &value) {
    using T = std::remove_reference_t<decltype(value)>;
    for (auto &c : str) {
        c = std::tolower(c);
    }
    if (str == "-inf") {
        value = std::numeric_limits<T>::min();
    }
    else if (str == "inf") {
        value = std::numeric_limits<T>::max();
    }
    else {
        auto begin = str.data(), end = str.data() + str.size();
        auto [ptr, ec] = std::from_chars(begin, end, value);
        if (ec != std::errc{} || ptr != end) {
            throw std::runtime_error("Invalid value: " + str);
        }
    }
}
inline const std::regex range_pattern(R"(\s*(\[|\()\s*(.+)\s*,\s*(.+)(\]|\))\s*)");
template<typename T> auto parse_range(const std::string &range) {
    std::pair<T, int64_t> L = {}, R = {};
    std::smatch result;
    if (std::regex_match(range, result, range_pattern)) {
        value_conv(result.str(2), L.first);
        value_conv(result.str(3), R.first);
        if (result.str(1) == "(") {
            L.second = std::numeric_limits<int64_t>::max();
        }
        if (result.str(4) == "]") {
            R.second = std::numeric_limits<int64_t>::max();
        }
    }
    else {
        throw std::runtime_error("Invalid range: " + range);
    }
    return std::make_pair(L, R);
}
inline auto parse_uint_range(const std::string &range) {
    std::smatch result;
    int64_t L = 1, R = 0;
    if (std::regex_match(range, result, range_pattern)) {
        value_conv(result.str(2), L);
        value_conv(result.str(3), R);
        if (result.str(1) == "(") {
            L += 1;
        }
        if (result.str(4) == "]") {
            R += 1;
        }
    }
    if (L > R || L < 0) {
        throw std::runtime_error("Invalid range: " + range);
    }
    return std::make_pair(L, R);
}
#endif