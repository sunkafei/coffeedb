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
    system("rm -r backup");
    system("rm -r raw");
    json a;
    init();
    auto response = [](auto &&message) {
        try {
            print(::response(message));
        }
        catch (std::exception &e) {
            std::string message = std::format("[ERROR] {}.", e.what());
            error(message);
        }
    };
    response(R"({
        "operation":"clear"
    })"_json);
    response(R"({
        "operation":"insert", 
        "data": {
            "id": "abcd",
            "range": 123
        }
    })"_json);
    response(R"({
        "operation":"insert", 
        "data":{
            "id": "efgcd7",
            "name": "smz",
            "range": 999
        }
    })"_json);
    response(R"({
        "operation":"build"
    })"_json);
    response(R"({
        "operation": "query",
        "constraints": {
            "id": "cd"
        },
        "fields": ["id"]
    })"_json);
    //response(R"({"operation":"query", "constraints":{"id":"[1,20]"},"fields":["id"]})"_json);
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