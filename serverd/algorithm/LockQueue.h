#pragma once

#include <deque>
#include <mutex>
#include <vector>
#include <utility>

template <typename T>
class LockQueue
{
public:
    explicit LockQueue(size_t capacity) : m_capacity(capacity) {}

    bool push(T item)
    {
        std::lock_guard<std::mutex> g(m_lock);
        if (m_capacity != 0 && m_q.size() >= m_capacity)
        {
            return false;
        }
        m_q.emplace_back(std::move(item));
        return true;
    }

    void popAll(std::vector<T>& out)
    {
        out.clear();
        std::lock_guard<std::mutex> g(m_lock);
        if (m_q.empty())
        {
            return;
        }
        out.reserve(m_q.size());
        while (!m_q.empty())
        {
            out.emplace_back(std::move(m_q.front()));
            m_q.pop_front();
        }
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> g(m_lock);
        return m_q.empty();
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> g(m_lock);
        return m_q.size();
    }

private:
    size_t m_capacity;
    mutable std::mutex m_lock;
    std::deque<T> m_q;
};
