/*
#pragma once

#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>

struct ThreadInfo {
    std::string name;
    std::thread thread;
    std::function<void()> stopFunc;
};

class ThreadManager {
public:
    ThreadManager() = default;

    ~ThreadManager();

    static bool setName(const std::string &name);

    void addThread(const std::string &name, std::function<void()> originFunc,
                   std::function<void()> stopFunc);

    void stopAll();

private:
    void threadWrapper(const std::string &name, std::function<void()> func);

    std::vector <ThreadInfo> m_threadList;
    std::mutex m_mtx;
};
*/

#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <string>

class ThreadManager
{
public:
    ~ThreadManager();

    void addThread(const std::string& name,
                   std::function<void()> runFunc,
                   std::function<void()> stopFunc = {});

    void start(size_t n,
               const std::string& baseName,
               std::function<void()> runFunc,
               std::function<void()> stopFunc = {});

    void stopAll();
    void join();

    static bool setName(const std::string& name);

private:
    struct ThreadInfo
    {
        std::string name;
        std::thread thread;
        std::function<void()> stopFunc;
    };

    void threadWrapper(const std::string& name,
                       std::function<void()> func);

private:
    std::vector<ThreadInfo> m_threads;
    std::mutex m_mtx;
};
