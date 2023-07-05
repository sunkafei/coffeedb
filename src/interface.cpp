#include <fstream>
#include <chrono>
#include <string>
#include <filesystem>
#include <cstdint>
#include <vector>
#include <format>
#include "interface.h"
#include "utility.h"
#include "config.h"
#include "database.h"
void backup(int64_t timestamp, const json &command) {
    std::ofstream file(storage_location + backup_directory + std::to_string(timestamp));
    file << command << std::endl;
}
void init() {
    std::filesystem::create_directory(storage_location + backup_directory);
    std::filesystem::create_directory(storage_location + backup_directory);
}
template<class Facet> struct deletable_facet : Facet {
    template<class... Args> deletable_facet(Args&&... args) : Facet(std::forward<Args>(args)...) {}
    ~deletable_facet() {}
};
void evaluate(const json &command) {
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    auto operation = command.at("operation");
    auto data = command.at("data");
    std::wstring_convert<deletable_facet<std::codecvt<char32_t, char, std::mbstate_t>>, char32_t> conv32;
    backup(timestamp, command);
    if (operation == "insert") {
        std::vector<std::pair<std::string, var>> object;
        for (const auto& item : data.items()) {
            if (item.value().is_number_integer()) {
                object.emplace_back(item.key(), item.value().template get<int64_t>());
            }
            else if (item.value().is_number_float()) {
                object.emplace_back(item.key(), item.value().template get<double>());
            }
            else if (item.value().is_string()) {
                auto u8str = item.value().template get<std::string>();
                auto u32str = conv32.from_bytes(u8str); //UTF-8 to UTF-32
                object.emplace_back(item.key(), u32str);
            }
            else {
                throw std::invalid_argument(std::format("Unrecognized object: {}", item.value().dump()));
            }
        }
        insert(timestamp, object);
    }
}