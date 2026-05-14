#pragma once
#include "router.hpp"
#include "thread_pool.hpp"

class Server {
public:
    explicit Server(int port);
    ~Server();

    void run(const Router& router);

private:
    int        port_;
    int        listen_fd_{-1};
    ThreadPool pool_;

    void handle_client(int client_fd, const Router& router);
};
