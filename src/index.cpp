//#include <execution>
#include <algorithm>
#include <regex>
#include <cctype>
#include <limits>
#include <string_view>
#include <bit>
#include <mutex>
#include <condition_variable>
#include <random>
#include <queue>
#include <thread>
#include <ranges>
#include <tuple>
#include "utility.h"
#include "index.h"
std::regex range_pattern(R"(\s*(\[|\()\s*(.+)\s*,\s*(.+)(\]|\))\s*)");
std::mutex mutex;
std::condition_variable condition_variable;
std::default_random_engine engine(20140920);
std::queue<std::tuple<void*, void*, int64_t>> segments;
int64_t chuck_size, rest;
void integer_index::add(int64_t id, int64_t value) {
    data.emplace_back(value, id);
}
void integer_index::build() {
    std::sort(data.begin(), data.end());
    data.shrink_to_fit();
}
void double_index::add(int64_t id, double value) {
    data.emplace_back(value, id);
}
void double_index::build() {
    std::sort(data.begin(), data.end());
    data.shrink_to_fit();
}
template<typename T> void string_index::parallel_sort() {
    int32_t right[256 + 8], pos[256 + 8];
    for (;;) {
        std::unique_lock lock(mutex);
        condition_variable.wait(lock, []{ return !segments.empty() || rest == 0; });
        if (rest == 0) {
            break;
        }
        auto [void_begin, void_end, offset] = segments.front();
        auto begin = (T*)void_begin, end = (T*)void_end;
        const auto length = end - begin;
        segments.pop();
        if (length <= chuck_size) {
            rest -= length;
        }
        lock.unlock();
        if (length <= chuck_size) {
            std::sort(begin, end, [this, offset](auto i, auto j) noexcept {
                return suffix(i, offset) < suffix(j, offset);
            });
            continue;
        }
        else {
            std::ranges::fill(right, 0);
            for (auto iter = begin; iter != end; ++iter) {
                right[character(*iter, offset)] += 1;
            }
            for (int i = 1; i < std::size(right); ++i) {
                right[i] += right[i - 1];
            }
            std::copy(std::begin(right), std::end(right), pos);
            int32_t now = 0;
            for (int64_t i = 0; i < length; ++i) {
                while (i == right[now]) {
                    now += 1;
                }
                for (;;) {
                    auto c = character(begin[i], offset);
                    if (c == now) {
                        break;
                    }
                    pos[c] -= 1;
                    std::swap(begin[i], begin[pos[c]]);
                }
            }
            std::lock_guard guard(mutex);
            rest -= right[0];
            for (int i = 1; i < std::ssize(right); ++i) {
                if (right[i] - right[i - 1] > 0) {
                    segments.emplace(begin + right[i - 1], begin + right[i], offset + 1);
                }
            }
        }
        condition_variable.notify_one();
        condition_variable.notify_one();
    }
}
void string_index::add(int64_t id, std::string_view value) {
    ids.push_back(id);
    data.emplace_back(value);
}
void string_index::build() {
    ids.shrink_to_fit();
    data.shrink_to_fit();
    this->size = 0;
    uint64_t mask1 = 1, mask2 = 1;
    while (mask1 < data.size()) {
        mask1 = (mask1 << 1) + 1;
    }
    for (uint64_t i = 0; i < data.size(); ++i) {
        size += data[i].size();
        while (mask2 < data[i].size()) {
            mask2 = (mask2 << 1) + 1;
        }
    }
    const auto bits1 = std::popcount(mask1);
    const auto bits2 = std::popcount(mask2);
    if (bits1 + bits2 > 64) {
        throw std::runtime_error("The amount of data exceeds the maximum range that CoffeeDB can handle");
    }
    if (bits1 > 32) {
        throw std::runtime_error("The number of objects exceeds the maximum range that CoffeeDB can handle");
    }
    this->mask = mask1;
    this->bits = bits1;
    if (bits1 + bits2 <= 32) {
        this->sa = new uint32_t[size];
    }
    else {
        this->sa = new uint64_t[size];
    }
    std::visit([this]<typename T>(T *sa) {
        auto *pointer = sa;
        for (T i = 0; i < data.size(); ++i) {
            for (T j = 0; j < data[i].size(); ++j) {
                *pointer++ = ((j << bits) | i);
            }
        }
        while (segments.size()) {
            segments.pop();
        }
        segments.emplace(sa, sa + size, 0);
        chuck_size = std::max((uint64_t)2048, this->size / 128);
        rest = this->size;
        auto worker = [this](){
            parallel_sort<T>();
        };
        std::vector<std::thread> threads;
        for (int i = 0; i + 1 < std::thread::hardware_concurrency(); ++i) {
            threads.emplace_back(worker);
        }
        worker();
        for (auto &thread : threads) {
            thread.join();
        }
        /*std::sort(std::execution::par, sa, sa + size, [this](auto i, auto j) noexcept {
            return suffix(i) < suffix(j);
        });*/
    }, sa);
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
    std::visit([this, &ret, &keyword](auto *sa){
        std::string_view keyword_view(keyword);
        int64_t L = 0, R = size - 1;
        while (L < R) {
            int64_t M = L + (R - L) / 2;
            auto i = sa[M];
            std::string_view content = suffix(i);
            if (keyword_view <= content) {
                R = M;
            }
            else {
                L = M + 1;
            }
        }
        int64_t left = L;
        L = left - 1, R = size - 1;
        while (L < R) {
            int64_t M = L + (R - L + 1) / 2;
            auto i = sa[M];
            std::string_view content = suffix(i);
            if (content.size() >= keyword_view.size() && keyword_view == content.substr(0, keyword_view.size())) {
                L = M;
            }
            else {
                R = M - 1;
            }
        }
        int64_t right = L + 1;
        if (left < right) {
            std::vector<uint64_t> indices;
            indices.reserve(right - left + 1);
            for (int64_t i = left; i < right; ++i) {
                indices.push_back(sa[i] & mask);
            }
            constexpr uint64_t base = (1 << 17) - 1;
            const uint64_t n = indices.size();
            if (n < base) {
                std::sort(indices.begin(), indices.end());
            }
            else { // RadixSort
                std::vector<uint64_t> c(base + 8, 0);
                std::vector<uint64_t> tmp(n, 0);
                for (int j = 0; j < n; ++j)
                    c[indices[j] & base]++;
                for (int j = 1; j <= base; ++j)
                    c[j] += c[j - 1];
                for (int j = n - 1; j >= 0; --j)
                    tmp[--c[indices[j] & base]] = indices[j];
                c = std::vector<uint64_t>(base + 8, 0);
                for (int j = 0; j < n; ++j)
                    c[(tmp[j] >> 16) & base]++;
                for (int j = 1; j <= base; ++j)
                    c[j] += c[j - 1];
                for (int j = n - 1; j >= 0; --j)
                    indices[--c[(tmp[j] >> 16) & base]] = tmp[j];
            }
            indices.push_back(std::numeric_limits<uint64_t>::max());
            uint64_t last = 0;
            for (uint64_t last = 0, i = 1; i < indices.size(); ++i) {
                if (indices[i] != indices[i - 1]) {
                    ret.emplace_back(ids[indices[last]], i - last);
                    last = i;
                }
            }
        }  
    }, sa);
    return ret;
}