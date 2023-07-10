#include <regex>
#include <format>
#include "config.h"
#include "utility.h"
#include "database.h"
void parse_command(int argc, char *argv[]) {
    std::regex pattern1(R"(--(\w+)=(.+))");
    std::regex pattern2(R"(--(\w+))");
    for (int i = 1; i < argc; ++i) {
        std::string argument(argv[i]);
        std::smatch result;
        std::string key, value;
        if (std::regex_match(argument, result, pattern1)) {
            key = result.str(1);
            value = result.str(2);
        }
        else if (std::regex_match(argument, result, pattern2)) {
            key = result.str(1);
        }
        else {
            throw std::runtime_error(std::format("Invalid command line argument: {}", argument));
        }
        if (key == "port") {
            value_conv(value, ::port);
        }
        else if (key == "clear") {
            clear();
        }
        else if (key == "directory") {
            if (!std::filesystem::exists(value)) {
                throw std::runtime_error("Invalid path: " + value);
            }
            if (!value.ends_with("/")) {
                value += "/";
            }
            ::storage_location = value;
        }
    }
}