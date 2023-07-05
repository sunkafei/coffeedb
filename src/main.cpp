#include <stdio.h>
#include "utility.h"
#include "interface.h"
auto main(int argc, char** argv) -> int {
    using namespace nlohmann::literals;
    json a;
    evaluate(R"(
    {
        "type": "insert",
        "data": {
            "A": 123,
            "B": "qwe"
        }
    }
    )"_json);
    return 0;
}
