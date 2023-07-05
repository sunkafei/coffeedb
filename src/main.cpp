#include <stdio.h>
#include "utility.h"
#include "interface.h"
auto main(int argc, char** argv) -> int {
    using namespace nlohmann::literals;
    json a;
    init();
    evaluate(R"(
    {
        "operation": "insert",
        "data": {
            "A": 123,
            "B": "qwe孙明志",
            "C": 3.14
        }
    }
    )"_json);
    return 0;
}
