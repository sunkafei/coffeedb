#include <stdio.h>
#include "utility.h"
#include "interface.h"
#include "server.h"
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
        "operation": "remove",
        "data": {
            "A" : "[100,199]",
            "B": "010"
        },
        "select" : ["A", "B", "C"]
    }
    )"_json);
    print(result);
}
int main() {
    //test();
    start_server();
    return 0;
}