#pragma once
#include <vector>
#include <thread>
#include <functional>

class ThreadManager {
public:
    ThreadManager() = default;
    ~ThreadManager();

    void start(size_t n, std::function<void()> fn);
    void join();

private:
    std::vector<std::thread> m_threads;
};

