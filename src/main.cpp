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
            "B": "qwe孙明志",
            "C": 1.14
        }
    }
    )"_json);
    response(R"(
    {
        "operation": "insert",
        "data": {
            "A": 200,
            "B": "123孙明志",
            "C": 2.14
        }
    }
    )"_json);
    auto result = response(R"(
    {
        "operation": "query",
        "data": {
            "B": "3孙明志"
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