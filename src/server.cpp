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
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    std::string ip = "127.0.0.1";
    char buffer[1024];
    FILE *stream = _popen("ipconfig", "r");
    while (fgets(buffer, std::ssize(buffer), stream)) {
        std::string line(buffer);
        if (line.ends_with('\n')) {
            line.pop_back();
        }
        if (line.find("IPv4") != std::string::npos && line.rfind(':') != std::string::npos) {
            auto index = line.rfind(':') + 1;
            while (index < line.size() && std::isspace(line[index])) {
                index += 1;
            }
            ip = line.substr(index);
        }
    }
#else
    std::string ip;
    FILE *output = popen("dig +short myip.opendns.com @resolver1.opendns.com", "r");
    for (;;) {
        auto ch = fgetc(output);
        if (isspace(ch) || ch == EOF) {
            break;
        }
        ip.push_back(ch);
    }
    if (ip.empty() || ip.find(':') != std::string::npos)
        ip = "127.0.0.1";
#endif
    print("Starting CoffeeDB ...");
    init();
    build();
    if (!server.is_valid()) {
        throw std::runtime_error("Server has an error");
    }
    /*server.set_error_handler([](const httplib::Request&, httplib::Response &res) {
        auto message = std::format("<p>Error Status: <span style='color:red;'>{}</span></p>", res.status);
        res.set_content(message, "text/html");
    });*/
    server.Post("/coffeedb", [](const httplib::Request &req, httplib::Response &res) {
        try {
            std::string reply = response(json::parse(req.body));
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(reply, "application/json");
        }
        catch (std::exception &e) {
            std::string message = std::format("[Error] {}.", e.what());
            res.set_content(message.c_str(), "text/html");
            res.status = 500;
        }
    });
    auto response_get = [](const httplib::Request &req, httplib::Response &res) {
        constexpr char content[] = R"(
            <h1>CoffeeDB</h1>
            <p>The project address is: <a href="https://github.com/sunkafei/coffeedb">https://github.com/sunkafei/coffeedb</a></p>
            Please use the <b>POST</b> method to interact with CoffeeDB.
        )";
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(content, "text/html");
    };
    server.Get("/", response_get);
    server.Get("/coffeedb", response_get);
    print(std::format("Working directory: {}", ::storage_location));
    print(std::format("Running at http://{}:{}/coffeedb", ip, ::port));
    if (!server.listen("0.0.0.0", ::port)) {
        throw std::runtime_error(std::format("Failed to listen on port {}", ::port));
    }
}
