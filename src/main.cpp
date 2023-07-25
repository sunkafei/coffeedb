#include <stdexcept>
#include <format>
#include "utility.h"
#include "interface.h"
#include "database.h"
#include "server.h"
#include "command.h"
#include "profile.h"
void test() {
    using namespace nlohmann::literals;
    clear();
    init();
    json a;
    auto send = [](auto &&message) {
        try {
            print("==> ", ::response(message));
        }
        catch (std::exception &e) {
            std::string message = std::format("[ERROR] {}.", e.what());
            error(message);
        }
    };
    send(R"({
        "operation": "insert",
        "data": {
            "number": 123,
            "name": "sunkafei",
            "secret": "3010103"
        }
    })"_json);
    send(R"({
        "operation": "insert",
        "data": {
            "number": 234,
            "name": "yulemao",
            "position": 1.7724,
            "secret": "301022"
        }
    })"_json);
    send(R"({
        "operation": "insert",
        "data": {
            "bool": true
        }
    })"_json);
    send(R"({
        "operation": "build"
    })"_json);
    send(R"({
        "operation": "query",
        "constraints": {
            "number": "[100,200]"
        }
    })"_json);
    send(R"({
        "operation": "query",
        "constraints": {
            "number": "[100,900]"
        },
        "fields": ["name"]
    })"_json);
    send(R"({
        "operation": "query",
        "constraints": {
            "secret": "010"
        }
    })"_json);
    send(R"({
        "operation": "query",
        "constraints": {
            "secret": "010",
            "number": "[0,900]"
        },
        "fields": ["name", "secret"],
        "highlight": ["<b>", "</b>"],
        "span": " [0,1) "
    })"_json);
    send(R"({
        "operation": "query",
        "constraints": {
            "bool": "true"
        }
    })"_json);
    send(R"({
        "operation": "count"
    })"_json);
}
int main(int argc, char *argv[]) {
    // curl http://127.0.0.1:14920/coffeedb -X POST -d '{"operation":"clear"}'
    // curl http://127.0.0.1:14920/coffeedb -X POST -d '{"operation":"build"}'
    // curl http://127.0.0.1:14920/coffeedb -X POST -d '{"operation":"query", "constraints":{"id":"[1,20]"},"fields":["id"]}'
    try {
        parse_command(argc, argv);
        //test();
        //profile_string_process();
        start_server();
    }
    catch (std::exception &e) {
        std::string message = std::format("[Error] {}.", e.what());
        error(message);
    }
    return 0;
}