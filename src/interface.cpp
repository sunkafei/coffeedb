#include <fstream>
#include <chrono>
#include <string>
#include "interface.h"
#include "utility.h"
void backup(const json &command) {
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    std::ofstream file(directory + "backup/" + std::to_string(timestamp));
    file << timestamp << std::endl;
}
void evaluate(const json &command) {
    backup(command);
    auto type = command.at("type");
    auto data = command.at("data");
    if (type == "insert") {
        std::cout << data << std::endl;
        for (const auto &item : data.items()) {
            //print(item.key(), item.value());
        }
    }
}