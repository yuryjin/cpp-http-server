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

    // Called when no route matches — useful for static file serving
    void set_fallback(Handler h) { fallback_ = std::move(h); }

    HttpResponse handle(const HttpRequest& req) const {
        bool is_head = (req.method == "HEAD");
        // HEAD reuses GET handlers but strips the body
        const std::string route_method = is_head ? "GET" : req.method;

        auto mit = routes_.find(route_method);
        if (mit != routes_.end()) {
            auto pit = mit->second.find(req.path);
            if (pit != mit->second.end()) {
                auto res = pit->second(req);
                if (is_head) {
                    res.headers["Content-Length"] = std::to_string(res.body.size());
                    res.body.clear();
                }
                return res;
            }
        }
        if (fallback_) {
            auto res = fallback_(req);
            if (is_head) {
                res.headers["Content-Length"] = std::to_string(res.body.size());
                res.body.clear();
            }
            return res;
        }
        if (mit == routes_.end()) return HttpResponse::method_not_allowed();
        return HttpResponse::not_found();
    }

private:
    std::map<std::string, std::map<std::string, Handler>> routes_;
    Handler fallback_;
};
