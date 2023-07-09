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
json jsonify(const std::vector<std::pair<const std::string, var>>& object) {
    json j;
    for (const auto &[key, value] : object) {
        std::visit([&j, &key](auto && value) {
            j[key] = value;
        }, value);
    }
    return j;
}
auto filter(const json &data) {
    std::vector<std::pair<int64_t, int64_t>> result;
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
                std::vector<std::pair<int64_t, int64_t>> tmp;
                for (int i = 0, j = 0; i < ssize(now) && j < ssize(result); ) {
                    if (now[i].first == result[j].first) {
                        tmp.push_back(now[i]);
                        tmp.back().second += result[j].second;
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
    return result;
}
std::string response(const json &command) {
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    std::string operation = command.at("operation");
    backup(timestamp, command);
    if (operation == "insert") {
        auto data = command.at("data");
        std::vector<std::pair<std::string, var>> object;
        for (const auto& item : data.items()) {
            if (item.value().is_number_integer()) {
                object.emplace_back(item.key(), item.value().template get<int64_t>());
            }
            else if (item.value().is_number_float()) {
                object.emplace_back(item.key(), item.value().template get<double>());
            }
            else if (item.value().is_string()) {
                auto str = item.value().template get<std::string>();
                object.emplace_back(item.key(), str);
            }
            else {
                throw std::runtime_error(std::format("Unrecognized object: {}", item.value().dump()));
            }
        }
        insert(timestamp, object);
        return "";
    }
    else if (operation == "query") {
        auto data = command.at("data");
        auto result = filter(data);
        std::vector<std::string> keys;
        if (command.contains("select")) {
            keys = command["select"].template get<std::vector<std::string>>();
        }
        auto list = select(result, keys);
        auto ret = json::array();;
        for (const auto &object : list) {
            ret.push_back(jsonify(object));
        }
        return ret.dump();
    }
    else if (operation == "remove") {
        auto data = command.at("data");
        auto result = filter(data);
        for (auto [id, _] : result) {
            std::filesystem::remove(storage_location + raw_directory + std::format("{}", id));
        }
    }
    else if (operation == "build") {
        build();
    }
    else {
        throw std::runtime_error("Invalid query type: " + operation);
    }
    return "";
}
