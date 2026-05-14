#include "http.hpp"
#include "router.hpp"
#include "server.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main() {
    Router router;

    router.get("/", [](const HttpRequest&) {
        std::string html = read_file("static/index.html");
        if (html.empty()) html = "<h1>Hello from C++ HTTP Server!</h1>";
        return HttpResponse::ok(html);
    });

    router.get("/api/hello", [](const HttpRequest&) {
        return HttpResponse::json(R"({"message": "Hello, World!", "server": "cpp-http"})");
    });

    router.post("/api/echo", [](const HttpRequest& req) {
        return HttpResponse::json(req.body);
    });

    try {
        Server server(8080);
        server.run(router);
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}
