#include "http.hpp"
#include "router.hpp"
#include "server.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

static std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Serve files from static/ directory; blocks path traversal
static HttpResponse serve_static(const HttpRequest& req) {
    std::string path = req.path;

    // Block path traversal
    if (path.find("..") != std::string::npos)
        return HttpResponse::bad_request();

    // Map "/" → "/index.html"
    if (path == "/" || path.empty()) path = "/index.html";

    std::string fs_path = "static" + path;
    std::string content = read_file(fs_path);
    if (content.empty()) return HttpResponse::not_found();

    return HttpResponse::ok(std::move(content), mime_type(fs_path));
}

int main() {
    Router router;

    router.get("/api/hello", [](const HttpRequest&) {
        return HttpResponse::json(R"({"message": "Hello, World!", "server": "cpp-http"})");
    });

    router.post("/api/echo", [](const HttpRequest& req) {
        return HttpResponse::json(req.body);
    });

    // All unmatched GET requests → static files
    router.set_fallback(serve_static);

    try {
        Server server(8080);
        server.run(router);
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}
