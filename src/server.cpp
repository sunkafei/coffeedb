#include <stdexcept>
#include <format>
#include <httplib.h>
#include "interface.h"
#include "config.h"
httplib::Server server;
void start_server(int port, const std::string &directory) {
    print(std::format("Working directory: {}", storage_location));
    print(std::format("Running at 127.0.0.1:{}/{}", port, directory));
    init();
    server.Get("/" + directory, [](const httplib::Request &req, httplib::Response &res) {
        json input = json::parse(req.get_param_value("json"));
        try {
            std::string reply = response(input);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(reply, "application/json");
        }
        catch (std::exception error) {
            std::string message = std::format("<h1>Error 500</h1><p>%s</p>", error.what());
            res.set_content(message.c_str(), "text/html");
            res.status = 500;
        }
    });
    server.listen("0.0.0.0", port);
}
