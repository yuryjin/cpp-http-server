#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t n_threads) {
        if (n_threads == 0) throw std::invalid_argument("ThreadPool: n_threads must be > 0");
        workers_.reserve(n_threads);
        for (size_t i = 0; i < n_threads; ++i)
            workers_.emplace_back(&ThreadPool::worker_loop, this);
    }

    ~ThreadPool() {
        {
            std::unique_lock lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& w : workers_) w.join();
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock lock(mutex_);
            if (stop_) throw std::runtime_error("ThreadPool: enqueue on stopped pool");
            tasks_.push(std::move(task));
        }
        cv_.notify_one();
    }

    size_t size() const { return workers_.size(); }

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            // Catch exceptions so one bad request doesn't kill the worker
            try { task(); } catch (...) {}
        }
    }

    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mutex_;
    std::condition_variable           cv_;
    bool                              stop_{false};
};
