#include <locale>
#include <algorithm>
#include <regex>
#include <cctype>
#include <limits>
#include "utility.h"
#include "index.h"
std::regex range_pattern(R"(\s*(\[|\()\s*(.+)\s*,\s*(.+)(\]|\))\s*)");
template<class Facet> struct deletable_facet : Facet {
    template<class... Args> deletable_facet(Args&&... args) : Facet(std::forward<Args>(args)...) {}
    ~deletable_facet() {}
};
std::wstring_convert<deletable_facet<std::codecvt<char32_t, char, std::mbstate_t>>, char32_t> conv32;
void integer_index::add(int64_t id, int64_t value) {
    data.emplace_back(value, id);
}
void double_index::add(int64_t id, double value) {
    data.emplace_back(value, id);
}
void string_index::add(int64_t id, const std::string &value) {
    data.emplace_back(value, id);
}
void integer_index::build() {
    std::sort(data.begin(), data.end());
}
void double_index::build() {
    std::sort(data.begin(), data.end());
}
void string_index::build() {
    
}
std::vector<std::pair<int64_t, int64_t>> numeric_query(const auto &data, const std::string &range) {
    using T = std::decay_t<decltype(data[0].first)>;
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
        throw std::runtime_error("Invalid query: " + range);
    }
    auto begin = std::lower_bound(data.begin(), data.end(), L);
    auto end = std::lower_bound(data.begin(), data.end(), R);
    std::vector<std::pair<int64_t, int64_t>> ret;
    ret.reserve(end - begin);
    for (auto iter = begin; iter != end; ++iter) {
        ret.emplace_back(iter->second, 0);
    }
    return ret;
}
std::vector<std::pair<int64_t, int64_t>> integer_index::query(const std::string &range) {
    return numeric_query(data, range);
}
std::vector<std::pair<int64_t, int64_t>> double_index::query(const std::string &range) {
    return numeric_query(data, range);
}
std::vector<std::pair<int64_t, int64_t>> string_index::query(const std::string &keyword) {
    std::vector<std::pair<int64_t, int64_t>> ret;
    for (const auto [content, id] : data) {
        auto iter = content.begin();
        int correlation = 0;
        for (;;) {
            iter = std::search(iter, content.end(), keyword.begin(), keyword.end());
            if (iter == content.end()) {
                break;
            }
            ++iter;
            correlation += 1;
        }
        if (correlation) {
            ret.emplace_back(id, correlation);
        }
    }
    return ret;
}