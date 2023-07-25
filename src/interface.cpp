#include <fstream>
#include <chrono>
#include <string>
#include <filesystem>
#include <cstdint>
#include <vector>
#include <format>
#include <ranges>
#include <optional>
#include "interface.h"
#include "utility.h"
#include "config.h"
#include "database.h"
json jsonify(const auto& object) {
    json j;
    for (const auto &[key, value] : object) {
        std::visit([&j, &key](auto && value) {
            j[key] = value;
        }, value);
    }
    return j;
}
auto get_constraints(const json &data) {
    std::vector<std::pair<std::string, std::vector<std::string>>> ret;
    const auto &items = data.items();
    for (auto iter = items.begin(); iter != items.end(); ++iter) {
        const auto &item = *iter;
        if (item.value().is_array()) {
            ret.emplace_back(item.key(), item.value().template get<std::vector<std::string>>());
        }
        else if (item.value().is_string()) {
            ret.emplace_back(item.key(), std::vector<std::string>(1, item.value().template get<std::string>()));
        }
        else {
            throw std::runtime_error(std::format("The constraint type of \"{}\" must be string or array of strings", item.key()));
        }
    }
    return ret;
}
auto filter(const json &data) {
    std::optional<std::pair<int64_t, int64_t>> correlation_range;
    std::vector<std::pair<int64_t, int64_t>> answer;
    bool first = true;
    const auto &items = data.items();
    if (items.begin() == items.end()) { // No constraints
        return query();
    }
    for (auto iter = items.begin(); iter != items.end(); ++iter) {
        const auto &item = *iter;
        std::vector<std::string> ranges;
        if (item.key() == key_correlation) {
            correlation_range = parse_uint_range(item.value());
            continue;
        }
        if (item.value().is_array()) {
            for (const auto &range : item.value()) {
                if (!range.is_string()) {
                    throw std::runtime_error(std::format("The constraint list of \"{}\" cannot contain non-strings", item.key()));
                }
                ranges.push_back(range);
            }
        }
        else if (item.value().is_string()) {
            ranges.push_back(item.value());
        }
        else {
            throw std::runtime_error(std::format("The constraint type of \"{}\" must be string or array of strings", item.key()));
        }
        if (ranges.empty()) {
            throw std::runtime_error(std::format("The constraint list of \"{}\" cannot be empty", item.key()));
        }
        std::vector<std::pair<int64_t, int64_t>> result;
        for (int64_t i = 0; i < ssize(ranges); ++i) {
            if (i == 0) {
                result = query(item.key(), ranges[i]);
                std::ranges::sort(result);
            }
            else {
                std::vector<std::pair<int64_t, int64_t>> tmp;
                auto now = query(item.key(), ranges[i]);
                std::ranges::sort(now);
                int i = 0, j = 0;
                for (; i < ssize(now) && j < ssize(result); ) {
                    if (now[i].first == result[j].first) {
                        tmp.push_back(now[i]);
                        tmp.back().second += result[j].second;
                        i += 1;
                        j += 1;
                    }
                    else if (now[i] < result[j]) {
                        tmp.push_back(now[i]);
                        i += 1;
                    }
                    else {
                        tmp.push_back(result[j]);
                        j += 1;
                    }
                }
                while (i < ssize(now)) {
                    tmp.push_back(now[i++]);
                }
                while (j < ssize(result)) {
                    tmp.push_back(result[j++]);
                }
                result = std::move(tmp);
            }
        }
        if (first) {
            answer = std::move(result);
            first = false;
        }
        else {
            std::vector<std::pair<int64_t, int64_t>> tmp;
            for (int i = 0, j = 0; i < ssize(result) && j < ssize(answer); ) {
                if (result[i].first == answer[j].first) {
                    tmp.push_back(result[i]);
                    tmp.back().second += answer[j].second;
                    i += 1;
                    j += 1;
                }
                else if (result[i] < answer[j]) {
                    i += 1;
                }
                else {
                    j += 1;
                }
            }
            answer = std::move(tmp);
        }
    }
    if (correlation_range) {
        auto [L, R] = *correlation_range;
        auto iter = std::remove_if(answer.begin(), answer.end(), [L, R](auto pair) {
            return !(pair.second >= L && pair.second < R);
        });
        answer.erase(iter, answer.end());
    }
    std::sort(answer.begin(), answer.end(), [](auto x, auto y) {
        return x.second > y.second; // Sort in descending order of $correlation.
    });
    return answer;
}
std::string response(json command) {
    std::string ret = "{}";
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    if (!command.is_object()) {
        throw std::runtime_error("You should pass a json object to CoffeeDB");
    }
    std::string operation = command.at("operation");
    command.erase(command.find("operation"));
    if (operation == "insert") {
        auto data = command.at("data");
        std::vector<std::pair<std::string, var>> object;
        for (const auto& item : data.items()) {
            if (item.value().is_boolean()) {
                object.emplace_back(item.key(), item.value().template get<bool>());
            }
            else if (item.value().is_number_integer()) {
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
        command.erase(command.find("data"));
    }
    else if (operation == "query") {
        std::vector<std::pair<std::string, std::vector<std::string>>> constraints;
        std::vector<std::pair<int64_t, int64_t>> result;
        if (command.contains("constraints")) {
            result = filter(command.at("constraints"));
            constraints = get_constraints(command.at("constraints"));
            command.erase(command.find("constraints"));
        }
        else {
            result = query();
        }
        std::vector<std::string> fields;
        if (command.contains("fields")) {
            if (command["fields"].is_string()) {
                fields.push_back(command["fields"]);
            }
            else if (command["fields"].is_array()) {
                for (const auto &field : command["fields"]) {
                    if (!field.is_string()) {
                        throw std::runtime_error(std::format("The list of fields cannot contain non-strings"));
                    }
                    fields.push_back(field);
                }
            }
            else {
                throw std::runtime_error("The type of fields must be string or array of strings");
            }
            command.erase(command.find("fields"));
        }
        std::string left, right;
        if (command.contains("highlight")) {
            try {
                auto list = command.at("highlight").template get<std::vector<std::string>>();
                if (list.size() != 2) {
                    throw std::runtime_error("");
                }
                left = list[0];
                right = list[1];
            }
            catch (...) {
                throw std::runtime_error("The format of \"highlight\" must be [left-padding, right-padding]");
            }
            command.erase(command.find("highlight"));
        }
        else {
            constraints.clear();
        }
        if (command.contains("span")) {
            auto range = parse_uint_range(command.at("span"));
            if (range.first >= ssize(result)) {
                result.clear();
            }
            else {
                auto end = result.end();
                if (range.second < ssize(result)) {
                    end = result.begin() + range.second;
                }
                result = { result.begin() + range.first, end };
            }
            command.erase(command.find("span"));
        }
        auto list = select(result, fields, constraints, left, right);
        auto L = json::array();
        for (const auto &object : list) {
            L.push_back(jsonify(object));
        }
        ret = L.dump();
    }
    else if (operation == "remove") {
        if (!command.contains("constraints")) {
            throw std::runtime_error("For security, the remove operation must have a \"constraints\" field");
        }
        auto constraints = command.at("constraints");
        auto result = filter(constraints);
        remove(result);
        command.erase(command.find("constraints"));
        json j;
        j["count"] = result.size();
        ret = j.dump();
    }
    else if (operation == "build") {
        build();
    }
    else if (operation == "count") {
        std::vector<std::pair<int64_t, int64_t>> result;
        if (command.contains("constraints")) {
            auto result = filter(command.at("constraints"));
            command.erase(command.find("constraints"));
        }
        else {
            result = query();
        }
        json j;
        j["count"] = result.size();
        ret = j.dump();
    }
    else if (operation == "clear") {
        clear();
    }
    else {
        throw std::runtime_error("Invalid operation: " + operation);
    }
    for (const auto &item : command.items()) {
        throw std::runtime_error(std::format("Invalid key: \"{}\"", item.key()));
    }
    return ret;
}
