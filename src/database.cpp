#include <vector>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <type_traits>
#include <cstdint>
#include "config.h"
#include "utility.h"
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
                const auto type = int8_t(type::integer);
                fwrite(&type, sizeof(type), 1, fp);
                fwrite(&value, sizeof(value), 1, fp);
            }
            else if constexpr (std::is_same_v<T, double>) {
                const auto type = int8_t(type::real);
                fwrite(&type, sizeof(type), 1, fp);
                fwrite(&value, sizeof(value), 1, fp);
            }
            else if constexpr (std::is_same_v<T, std::u32string>) {
                const auto type = int8_t(type::string);
                const int32_t length = value.size();
                fwrite(&type, sizeof(type), 1, fp);
                fwrite(&length, sizeof(length), 1, fp);
                fwrite(value.data(), sizeof(char32_t), length, fp);
            }
            else {
                static_assert(false, "Non-exhaustive visitor!");
            }
        }, value);
    }
    fclose(fp);
}