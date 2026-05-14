#include "server.hpp"
#include "http.hpp"
#include "logger.hpp"
#include <chrono>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>

static size_t default_pool_size() {
    size_t n = std::thread::hardware_concurrency();
    return n > 0 ? n : 4;
}

Server::Server(int port)
    : port_(port), pool_(default_pool_size()) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed — port already in use?");

    if (listen(listen_fd_, 128) < 0)
        throw std::runtime_error("listen() failed");
}

Server::~Server() {
    if (listen_fd_ >= 0) close(listen_fd_);
}

void Server::run(const Router& router) {
    std::cout << "Listening on http://localhost:" << port_
              << "  [thread pool: " << pool_.size() << "]\n" << std::flush;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);

        if (client_fd < 0) {
            std::cerr << "accept() failed\n";
            continue;
        }

        pool_.enqueue([this, client_fd, &router]() {
            handle_client(client_fd, router);
        });
    }
}

static size_t parse_content_length(const std::string& headers_block) {
    auto pos = headers_block.find("Content-Length: ");
    if (pos == std::string::npos) return 0;
    return std::stoul(headers_block.substr(pos + 16));
}

static bool wants_keep_alive(const HttpRequest& req) {
    // HTTP/1.1 defaults to keep-alive, HTTP/1.0 defaults to close
    bool ka = (req.version == "HTTP/1.1");
    auto it = req.headers.find("Connection");
    if (it != req.headers.end()) {
        if (it->second == "close")      ka = false;
        if (it->second == "keep-alive") ka = true;
    }
    return ka;
}

void Server::handle_client(int client_fd, const Router& router) {
    // Idle timeout so keep-alive connections don't hold a thread forever
    timeval tv{5, 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::array<char, 8192> buf;
    std::string carry;  // leftover bytes from a previous read (pipelining)

    while (true) {
        // --- Read headers ---
        std::string raw = std::move(carry);
        carry.clear();

        while (raw.find("\r\n\r\n") == std::string::npos) {
            ssize_t n = recv(client_fd, buf.data(), buf.size(), 0);
            if (n <= 0) { close(client_fd); return; }
            raw.append(buf.data(), static_cast<size_t>(n));
        }

        // --- Read body ---
        auto header_end   = raw.find("\r\n\r\n");
        size_t body_start = header_end + 4;
        size_t content_length = parse_content_length(raw.substr(0, header_end));

        while (raw.size() - body_start < content_length) {
            ssize_t n = recv(client_fd, buf.data(), buf.size(), 0);
            if (n <= 0) break;
            raw.append(buf.data(), static_cast<size_t>(n));
        }

        // Any bytes past this request belong to the next one
        size_t msg_end = body_start + content_length;
        if (raw.size() > msg_end)
            carry = raw.substr(msg_end);

        // --- Handle ---
        auto started_at = std::chrono::steady_clock::now();
        HttpRequest req  = parse_request(raw.substr(0, msg_end));
        bool keep_alive  = wants_keep_alive(req);

        HttpResponse res = router.handle(req);
        res.headers["Connection"] = keep_alive ? "keep-alive" : "close";
        if (keep_alive)
            res.headers["Keep-Alive"] = "timeout=5, max=100";

        Logger::instance().log_request(req.method, req.path, res.status_code, started_at);
        std::string response = res.serialize();
        send(client_fd, response.c_str(), response.size(), 0);

        if (!keep_alive) break;
    }

    close(client_fd);
}
