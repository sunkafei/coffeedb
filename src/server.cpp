#include <stdexcept>
#include <format>
#include <cstdio>
#include <httplib.h>
#include <cctype>
#include "config.h"
#include "interface.h"
#include "database.h"
httplib::Server server;
void start_server() {
    FILE *output = popen("dig +short myip.opendns.com @resolver1.opendns.com", "r");
    std::string ip;
    for (;;) {
        auto ch = fgetc(output);
        if (isspace(ch) || ch == EOF) {
            break;
        }
        ip.push_back(ch);
    }
    if (ip.empty() || ip.find(':') != std::string::npos)
        ip = "127.0.0.1";
    print("Starting CoffeeDB ...");
    init();
    build();
    print(std::format("Working directory: {}", ::storage_location));
    print(std::format("Running at http://{}:{}/coffeedb", ip, ::port));
    server.Post("/coffeedb", [](const httplib::Request &req, httplib::Response &res) {
        try {
            json input = json::parse(req.body);
            std::string reply = response(input);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(reply, "application/json");
        }
        catch (std::exception &e) {
            std::string message = std::format("[Error] {}.", e.what());
            res.set_content(message.c_str(), "text/html");
            res.status = 500;
        }
    });
    constexpr auto content = "<h1>CoffeeDB</h1><p>https://github.com/sunkafei/coffeedb</p>";
    server.Get("/", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(content, "text/html");
    });
    server.Get("/coffeedb", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(content, "text/html");
    });
    if (!server.listen("0.0.0.0", ::port)) {
        throw std::runtime_error(std::format("Failed to listen on port {}", ::port));
    }
}
