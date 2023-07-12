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
    response(R"(
    {
        "operation": "insert",
        "data": {
            "A": 100,
            "B": "010孙咖啡",
            "C": 111
        }
    }
    )"_json);
    response(R"(
    {
        "operation": "insert",
        "data": {
            "A": 200,
            "B": "123孙咖啡0101010",
            "C": 2.14
        }
    }
    )"_json);
    /*response(R"(
        {
            "operation": "build"
        }
    )"_json);
    auto result = response(R"(
    {
        "operation": "query",
        "constraints": {
            "B": "01010"
        },
        "fields": ["$correlation", "A"]
    }
    )"_json);
    print(result);*/
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