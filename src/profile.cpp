#include <chrono>
#include <string>
#include <random>
#include "interface.h"
void run(const std::string &command) {
    auto ret = ::system(command.c_str());
    if (ret != 0) {
        throw std::runtime_error("Failed to run: " + command);
    }
}
void profile_string_process() {
    long long total = 0;
    std::string data(10000, '*');
    std::default_random_engine engine;
    std::uniform_int_distribution<int> gen('a', 'z');
    run("free -h");
    response(R"(
        {
            "operation": "clear"
        }
    )"_json);
    auto response = [&total](auto val) {
        auto start = std::chrono::steady_clock::now();
        auto ret = ::response(std::move(val));
        auto end = std::chrono::steady_clock::now();
        total += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        return ret;
    };
    for (int _ = 0; _ < 10000; ++_) {
        for (int i = 0; i < ssize(data); ++i) {
            data[i] = gen(engine);
        }
        auto j = R"({
            "operation": "insert",
            "data": {
                "id": 1,
                "data": "0"
            }
        })"_json;
        j["data"]["data"] = data;
        response(j);
    }
    print(std::format("Insert Time: {}ms", total));
    total = 0;
    response(R"(
        {
            "operation": "build"
        }
    )"_json);
    print(std::format("Build Time: {}ms", total));
    run("free -h");
    total = 0;
    data.resize(4);
    for (int _ = 0; _ < 50; ++_) {
        for (int i = 0; i < ssize(data); ++i) {
            data[i] = gen(engine);
        }
        auto j = R"({
            "operation": "query",
            "constraints": {
                "data": "0"
            },
            "fields": [
                "id"
            ]
        })"_json;
        j["constraints"]["data"] = data;
        auto ret = response(std::move(j));
    }
    print(std::format("Query Time: {}ms", total));
}
/*
Insert Time: 10ms 
Build Time: 221ms 
Query Time: 3941ms 
*/