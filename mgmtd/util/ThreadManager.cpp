#include "ThreadManager.h"

ThreadManager::~ThreadManager() 
{
    join();
}

void ThreadManager::start(size_t n, std::function<void()> fn) 
{
    for (size_t i = 0; i < n; ++i) 
    {
        m_threads.emplace_back(fn);
    }
}

void ThreadManager::join() 
{
    for (auto& t : m_threads) {
        if (t.joinable())
            t.join();
    }
    m_threads.clear();
}

