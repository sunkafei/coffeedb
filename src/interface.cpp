#include <fstream>
#include <chrono>
#include <string>
#include <filesystem>
#include <cstdint>
#include <vector>
#include <format>
#include <ranges>
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
    std::filesystem::create_directory(storage_location + raw_directory);
}
json jsonify(const std::vector<std::pair<const std::string, var>*>& object) {
    json j;
    for (auto ptr : object) {
        const auto &[key, value] = *ptr;
        std::visit([&j, &key](auto && value) {
            j[key] = value;
        }, value);
    }
    return j;
}
std::string response(const json &command) {
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    auto operation = command.at("operation");
    auto data = command.at("data");
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
                object.emplace_back(item.key(), u8str);
            }
            else {
                throw std::runtime_error(std::format("Unrecognized object: {}", item.value().dump()));
            }
        }
        insert(timestamp, object);
        return "";
    }
    else if (operation == "query") {
        std::vector<int64_t> result;
        const auto &items = data.items();
        for (auto iter = items.begin(); iter != items.end(); ++iter) {
            const auto &item = *iter;
            if (item.value().is_string()) {
                auto str = item.value().template get<std::string>();
                auto now = query(item.key(), item.value());
                std::ranges::sort(now);
                if (iter == items.begin()) {
                    result = std::move(now);
                }
                else {
                    std::vector<int64_t> tmp;
                    for (int i = 0, j = 0; i < ssize(now) && j < ssize(result); ) {
                        if (now[i] == result[j]) {
                            tmp.push_back(now[i]);
                            i += 1;
                            j += 1;
                        }
                        else if (now[i] < result[j]) {
                            i += 1;
                        }
                        else {
                            j += 1;
                        }
                    }
                    result = std::move(tmp);
                }
            }
            else {
                throw std::runtime_error("Query type must be string");
            }
        }
        std::vector<std::string> keys;
        if (command.contains("select")) {
            keys = command["select"].template get<std::vector<std::string>>();
        }
        auto list = select(result, keys);
        json ret;
        for (const auto &object : list) {
            ret.push_back(jsonify(object));
        }
        return ret.dump();
    }
    return "";
}
