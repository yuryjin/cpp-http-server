#include "server.hpp"
#include "http.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>

Server::Server(int port) : port_(port) {
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
    std::cout << "Listening on http://localhost:" << port_ << "\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);

        if (client_fd < 0) {
            std::cerr << "accept() failed\n";
            continue;
        }

        std::thread([this, client_fd, &router]() {
            handle_client(client_fd, router);
        }).detach();
    }
}

void Server::handle_client(int client_fd, const Router& router) {
    std::string raw;
    std::array<char, 8192> buf;

    // Read until headers are fully received
    while (raw.find("\r\n\r\n") == std::string::npos) {
        ssize_t n = recv(client_fd, buf.data(), buf.size(), 0);
        if (n <= 0) { close(client_fd); return; }
        raw.append(buf.data(), static_cast<size_t>(n));
    }

    // Read body based on Content-Length
    auto header_end = raw.find("\r\n\r\n");
    std::string headers_block = raw.substr(0, header_end);
    size_t content_length = 0;

    auto cl = headers_block.find("Content-Length: ");
    if (cl != std::string::npos)
        content_length = std::stoul(headers_block.substr(cl + 16));

    size_t body_received = raw.size() - (header_end + 4);
    while (body_received < content_length) {
        ssize_t n = recv(client_fd, buf.data(), buf.size(), 0);
        if (n <= 0) break;
        raw.append(buf.data(), static_cast<size_t>(n));
        body_received += static_cast<size_t>(n);
    }

    HttpRequest req = parse_request(raw);
    std::string response = router.handle(req).serialize();
    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
}
