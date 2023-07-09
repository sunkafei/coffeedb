#include <stdexcept>
#include <format>
#include <httplib.h>
#include "config.h"
#include "interface.h"
#include "database.h"
httplib::Server server;
void start_server(int port, const std::string &directory) {
    print("Starting CoffeeDB ...");
    init();
    build();
    print(std::format("Working directory: {}", storage_location));
    print(std::format("Running at 127.0.0.1:{}/{}", port, directory));
    server.Post("/" + directory, [](const httplib::Request &req, httplib::Response &res) {
        try {
            json input = json::parse(req.body);
            std::string reply = response(input);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(reply, "application/json");
        }
        catch (std::exception &error) {
            std::string message = std::format("<h1>Error 500</h1><p>{}</p>", error.what());
            res.set_content(message.c_str(), "text/html");
            res.status = 500;
        }
    });
    server.Get("/", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content("CoffeeDB", "application/json");
    });
    server.listen("0.0.0.0", port);
}
