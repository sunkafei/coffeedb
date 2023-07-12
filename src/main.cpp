#include <stdexcept>
#include <format>
#include "utility.h"
#include "interface.h"
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
            "C": 1.14
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
    response(R"(
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
    print(result);
}
int main(int argc, char *argv[]) {
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