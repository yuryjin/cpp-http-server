#include "http.hpp"
#include <sstream>
#include <unordered_map>

std::string HttpResponse::serialize() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    for (auto& [k, v] : headers)
        oss << k << ": " << v << "\r\n";
    if (headers.find("Content-Length") == headers.end())
        oss << "Content-Length: " << body.size() << "\r\n";
    oss << "\r\n" << body;
    return oss.str();
}

HttpResponse HttpResponse::ok(std::string body, std::string content_type) {
    HttpResponse r;
    r.body = std::move(body);
    r.headers["Content-Type"] = std::move(content_type);
    return r;
}

HttpResponse HttpResponse::json(std::string body) {
    return ok(std::move(body), "application/json");
}

HttpResponse HttpResponse::not_found() {
    HttpResponse r;
    r.status_code = 404;
    r.status_text = "Not Found";
    r.body = "<h1>404 Not Found</h1>";
    r.headers["Content-Type"] = "text/html; charset=utf-8";
    return r;
}

HttpResponse HttpResponse::method_not_allowed() {
    HttpResponse r;
    r.status_code = 405;
    r.status_text = "Method Not Allowed";
    r.body = "<h1>405 Method Not Allowed</h1>";
    r.headers["Content-Type"] = "text/html; charset=utf-8";
    return r;
}

HttpResponse HttpResponse::bad_request() {
    HttpResponse r;
    r.status_code = 400;
    r.status_text = "Bad Request";
    r.body = "<h1>400 Bad Request</h1>";
    r.headers["Content-Type"] = "text/html; charset=utf-8";
    return r;
}

HttpRequest parse_request(const std::string& raw) {
    HttpRequest req;

    auto header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) return req;

    std::istringstream stream(raw.substr(0, header_end));
    std::string line;

    if (!std::getline(stream, line)) return req;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream first_line(line);
    first_line >> req.method >> req.path >> req.version;

    auto q = req.path.find('?');
    if (q != std::string::npos) {
        req.query_string = req.path.substr(q + 1);
        req.path = req.path.substr(0, q);
    }

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            if (!val.empty() && val.front() == ' ') val = val.substr(1);
            req.headers[key] = val;
        }
    }

    req.body = raw.substr(header_end + 4);
    return req;
}

std::string mime_type(const std::string& path) {
    static const std::unordered_map<std::string, std::string> types = {
        {".html", "text/html; charset=utf-8"},
        {".css",  "text/css"},
        {".js",   "application/javascript"},
        {".json", "application/json"},
        {".png",  "image/png"},
        {".jpg",  "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif",  "image/gif"},
        {".svg",  "image/svg+xml"},
        {".ico",  "image/x-icon"},
        {".txt",  "text/plain"},
        {".xml",  "application/xml"},
        {".pdf",  "application/pdf"},
        {".woff2","font/woff2"},
        {".woff", "font/woff"},
    };
    auto dot = path.rfind('.');
    if (dot == std::string::npos) return "application/octet-stream";
    auto it = types.find(path.substr(dot));
    return it != types.end() ? it->second : "application/octet-stream";
}
