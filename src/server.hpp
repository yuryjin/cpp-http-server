#pragma once
#include "router.hpp"

class Server {
public:
    explicit Server(int port);
    ~Server();

    void run(const Router& router);

private:
    int port_;
    int listen_fd_{-1};

    void handle_client(int client_fd, const Router& router);
};
