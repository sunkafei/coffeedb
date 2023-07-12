#include <locale>
#include <algorithm>
#include <regex>
#include <cctype>
#include <limits>
#include <string_view>
#include <execution>
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
void string_index::add(int64_t id, std::string_view value) {
    ids.push_back(id);
    data.emplace_back(value);
}
void integer_index::build() {
    std::sort(data.begin(), data.end());
    data.shrink_to_fit();
}
void double_index::build() {
    std::sort(data.begin(), data.end());
    data.shrink_to_fit();
}
void string_index::build() {
    for (uint64_t i = 0; i < data.size(); ++i) {
        for (uint64_t j = 0; j < data[i].size(); ++j) {
            sa.emplace_back(i, j);
        }
    }
    ids.shrink_to_fit();
    data.shrink_to_fit();
    sa.shrink_to_fit();
    std::sort(std::execution::par, sa.begin(), sa.end(), [this](auto i, auto j) {
        return data[i.index1].substr(i.index2) < data[j.index1].substr(j.index2);
    });
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
    /*for (int i = 0; i < data.size(); ++i) {
        const auto &content = data[i];
        auto id = ids[i];
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
    return ret;*/
    std::string_view keyword_view(keyword);
    uint64_t L = 0, R = sa.size() - 1;
    while (L < R) {
        uint64_t M = L + (R - L) / 2;
        auto i = sa[M];
        std::string_view content = data[i.index1].substr(i.index2);
        if (keyword_view <= content) {
            R = M;
        }
        else {
            L = M + 1;
        }
    }
    uint64_t left = L;
    L = left - 1, R = sa.size() - 1;
    while (L < R) {
        uint64_t M = L + (R - L + 1) / 2;
        auto i = sa[M];
        std::string_view content = data[i.index1].substr(i.index2);
        if (content.size() >= keyword_view.size() && keyword_view == content.substr(0, keyword_view.size())) {
            L = M;
        }
        else {
            R = M - 1;
        }
    }
    uint64_t right = L + 1;
    if (left < right) {
        std::vector<uint32_t> indices;
        indices.reserve(right - left + 1);
        for (uint64_t i = left; i < right; ++i) {
            indices.push_back(sa[i].index1);
        }
        constexpr uint32_t base = (1 << 17) - 1;
        const uint32_t n = indices.size();
        if (n < base) {
            std::sort(indices.begin(), indices.end());
        }
        else { // RadixSort
            std::vector<uint32_t> c(base + 8, 0);
            std::vector<uint32_t> tmp(n, 0);
            for (int j = 0; j < n; ++j)
                c[indices[j] & base]++;
            for (int j = 1; j <= base; ++j)
                c[j] += c[j - 1];
            for (int j = n - 1; j >= 0; --j)
                tmp[--c[indices[j] & base]] = indices[j];
            c = std::vector<uint32_t>(base + 8, 0);
            for (int j = 0; j < n; ++j)
                c[(tmp[j] >> 16) & base]++;
            for (int j = 1; j <= base; ++j)
                c[j] += c[j - 1];
            for (int j = n - 1; j >= 0; --j)
                indices[--c[(tmp[j] >> 16) & base]] = tmp[j];
        }
        indices.push_back(std::numeric_limits<uint32_t>::max());
        uint64_t last = 0;
        for (uint64_t last = 0, i = 1; i < indices.size(); ++i) {
            if (indices[i] != indices[i - 1]) {
                ret.emplace_back(ids[indices[last]], i - last);
                last = i;
            }
        }
    }
    return ret;
}