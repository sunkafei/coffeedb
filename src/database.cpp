#include <vector>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <type_traits>
#include <cstdint>
#include <map>
#include <filesystem>
#include <memory>
#include <format>
#include <unordered_map>
#include <algorithm>
#include "config.h"
#include "utility.h"
#include "index.h"
constexpr std::string key_correlation("$correlation");
std::map<std::string, std::unique_ptr<index>> indices;
std::unordered_map<int64_t, std::map<std::string, var>> data;
void build() {
    std::map<std::string, std::unique_ptr<index>> indices;
    std::unordered_map<int64_t, std::map<std::string, var>> data;
    for (const auto &e : std::filesystem::directory_iterator(storage_location + raw_directory)) {
        auto path = e.path().lexically_normal().string();
        FILE *fp = fopen(path.c_str(), "rb");
        if (!fp) {
            throw std::runtime_error("Cannot open file: " + path);
        }
        bool success = true;
        int64_t id;
        int32_t size;
        success |= (fread(&id, sizeof(id), 1, fp) == sizeof(id));
        success |= (fread(&size, sizeof(size), 1, fp) == sizeof(size));
        if (size <= 0) {
            success = false;
        }
        for (int i = 0; i < size; ++i) {
            int32_t length;
            success |= (fread(&length, sizeof(length), 1, fp) == sizeof(length));
            if (length <= 0) {
                success = false;
                break;
            }
            std::string key(length, '*');
            success |= (fread(key.data(), sizeof(char), length, fp) == sizeof(char) * length);
            int8_t type;
            success |= (fread(&type, sizeof(type), 1, fp) == sizeof(type));
            if (type == integer_index::number) {
                int64_t value;
                success |= (fread(&value, sizeof(value), 1, fp) == sizeof(value));
                if (!indices.count(key)) {
                    indices[key] = std::make_unique<integer_index>();
                }
                auto *ptr = dynamic_cast<integer_index*>(indices[key].get());
                if (!ptr) {
                    throw std::runtime_error(std::format("Mismatched type for \"{}\"", key));
                }
                else {
                    ptr->add(id, value);
                    data[id][key] = value;
                }
            }
            else if (type == double_index::number) {
                double value;
                success |= (fread(&value, sizeof(value), 1, fp) == sizeof(value));
                if (!indices.count(key)) {
                    indices[key] = std::make_unique<double_index>();
                }
                auto *ptr = dynamic_cast<double_index*>(indices[key].get());
                if (!ptr) {
                    throw std::runtime_error(std::format("Mismatched type for \"{}\"", key));
                }
                else {
                    ptr->add(id, value);
                    data[id][key] = value;
                }
            }
            else if (type == string_index::number) {
                int32_t length;
                success |= (fread(&length, sizeof(length), 1, fp) == sizeof(length));
                if (length <= 0) {
                    success = false;
                    break;
                }
                std::string value(length, '*');
                success |= (fread(value.data(), sizeof(char), length, fp) == sizeof(char) * length);
                if (!indices.count(key)) {
                    indices[key] = std::make_unique<string_index>();
                }
                auto *ptr = dynamic_cast<string_index*>(indices[key].get());
                if (!ptr) {
                    throw std::runtime_error(std::format("Mismatched type for \"{}\"", key));
                }
                else {
                    ptr->add(id, std::move(value));
                    data[id][key] = value;
                }
            }
            else {
                success = false;
                break;
            }
        }
        if (!success) {
            throw std::runtime_error("Corrupted File: " + path);
        }
    }
    for (auto &[key, value] : indices) {
        value->build();
    }
    ::indices = std::move(indices);
    ::data = std::move(data);
}
void insert(int64_t id, const std::vector<std::pair<std::string, var>> &object) {
    std::string filename = storage_location + raw_directory + std::to_string(id);
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    int32_t size = object.size();
    fwrite(&id, sizeof(id), 1, fp);
    fwrite(&size, sizeof(size), 1, fp);
    for (int i = 0; i < size; ++i) {
        const auto &[key, value] = object[i];
        int32_t length = key.size();
        fwrite(&length, sizeof(length), 1, fp);
        fwrite(key.data(), sizeof(char), length, fp);
        std::visit([fp](auto &&value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, int64_t>) {
                const int8_t type = integer_index::number;
                fwrite(&type, sizeof(type), 1, fp);
                fwrite(&value, sizeof(value), 1, fp);
            }
            else if constexpr (std::is_same_v<T, double>) {
                const int8_t type = double_index::number;
                fwrite(&type, sizeof(type), 1, fp);
                fwrite(&value, sizeof(value), 1, fp);
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                const int8_t type = string_index::number;
                const int32_t length = value.size();
                fwrite(&type, sizeof(type), 1, fp);
                fwrite(&length, sizeof(length), 1, fp);
                fwrite(value.data(), sizeof(char), length, fp);
            }
            else {
                static_assert(false, "Non-exhaustive visitor!");
            }
        }, value);
    }
    fclose(fp);
}
std::vector<std::pair<int64_t, int64_t>> query(const std::string& key, const std::string& range) {
    if (!indices.count(key)) {
        return {};
    }
    auto result = indices[key]->query(range);
    return result;
}
std::vector<std::vector<std::pair<const std::string, var>>> select(const std::vector<std::pair<int64_t, int64_t>>& results, const std::vector<std::string> &keys) {
    std::vector<std::vector<std::pair<const std::string, var>>> ret;
    for (auto [id, correlation] : results) {
        std::vector<std::pair<const std::string, var>> object;
        if (keys.size()) {
            for (const auto &key : keys) {
                auto iter = data[id].find(key);
                if (iter != data[id].end()) {
                    object.push_back(*iter);
                }
            }
        }
        else {
            for (auto iter = data[id].begin(); iter != data[id].end(); ++iter) {
                object.push_back(*iter);
            }
        }
        if (correlation && (keys.empty() || std::find(keys.cbegin(), keys.cend(), key_correlation) != keys.end())) {
            object.emplace_back(key_correlation, correlation);
        }
        if (object.size()) {
            ret.push_back(std::move(object));
        }
    }
    return ret;
}
void clear() {
    std::filesystem::remove_all(storage_location + raw_directory);
    std::filesystem::create_directory(storage_location + raw_directory);
}