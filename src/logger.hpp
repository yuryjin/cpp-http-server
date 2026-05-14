#pragma once
#include <chrono>
#include <ctime>
#include <iostream>
#include <mutex>
#include <string>

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void log_request(const std::string& method, const std::string& path,
                     int status, std::chrono::steady_clock::time_point started_at) {
        using namespace std::chrono;
        double ms = duration<double, std::milli>(steady_clock::now() - started_at).count();

        std::lock_guard lock(mutex_);
        std::cout << "[" << timestamp() << "] "
                  << method << " " << path << " "
                  << status << " "
                  << ms << "ms\n" << std::flush;
    }

private:
    Logger() = default;
    std::mutex mutex_;

    static std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        return buf;
    }
};
