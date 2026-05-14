#pragma once
#include "http.hpp"
#include <functional>
#include <map>
#include <string>

using Handler = std::function<HttpResponse(const HttpRequest&)>;

class Router {
public:
    void get(const std::string& path, Handler h)  { routes_["GET"][path]  = std::move(h); }
    void post(const std::string& path, Handler h) { routes_["POST"][path] = std::move(h); }

    HttpResponse handle(const HttpRequest& req) const {
        auto mit = routes_.find(req.method);
        if (mit == routes_.end()) return HttpResponse::method_not_allowed();
        auto pit = mit->second.find(req.path);
        if (pit == mit->second.end()) return HttpResponse::not_found();
        return pit->second(req);
    }

private:
    std::map<std::string, std::map<std::string, Handler>> routes_;
};
