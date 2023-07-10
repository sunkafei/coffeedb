#include <stdexcept>
#include <format>
#include <httplib.h>
#include "config.h"
#include "interface.h"
#include "database.h"
httplib::Server server;
void start_server() {
    print("Starting CoffeeDB ...");
    init();
    build();
    print(std::format("Working directory: {}", ::storage_location));
    print(std::format("Running at 127.0.0.1:{}/coffeedb", ::port));
    server.Post("/coffeedb", [](const httplib::Request &req, httplib::Response &res) {
        try {
            json input = json::parse(req.body);
            std::string reply = response(input);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(reply, "application/json");
        }
        catch (std::exception &e) {
            std::string message = std::format("<h1>Error 500</h1><p>{}</p>", e.what());
            res.set_content(message.c_str(), "text/html");
            res.status = 500;
        }
    });
    server.Get("/", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content("CoffeeDB", "application/json");
    });
    if (!server.listen("0.0.0.0", ::port)) {
        throw std::runtime_error(std::format("Failed to listen on port {}", ::port));
    }
}
