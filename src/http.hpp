#pragma once
#include <map>
#include <string>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::string query_string;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code{200};
    std::string status_text{"OK"};
    std::map<std::string, std::string> headers;
    std::string body;

    std::string serialize() const;

    static HttpResponse ok(std::string body, std::string content_type = "text/html; charset=utf-8");
    static HttpResponse json(std::string body);
    static HttpResponse not_found();
    static HttpResponse method_not_allowed();
    static HttpResponse bad_request();
};

HttpRequest parse_request(const std::string& raw);
