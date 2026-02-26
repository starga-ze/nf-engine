#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <cstddef>
#include <cstdlib>

template<typename T>
class MpscQueue
{
public:
    explicit MpscQueue(size_t capacity)
        : m_capacity(capacity),
          m_mask(capacity - 1),
          m_buffer(std::make_unique<Slot[]>(capacity))
    {
        if (capacity == 0 || (capacity & (capacity - 1)) != 0)
        {
            std::abort();
        }

        for (size_t i = 0; i < capacity; ++i)
        {
            m_buffer[i].ready.store(false, std::memory_order_relaxed);
            m_buffer[i].ptr = nullptr;
        }
    }

    ~MpscQueue()
    {
        for (size_t i = 0; i < m_capacity; ++i)
        {
            delete m_buffer[i].ptr;
        }
    }

public:

    bool enqueue(std::unique_ptr<T> item)
    {
        size_t tail;

        while (true)
        {
            tail = m_tail.load(std::memory_order_relaxed);
            size_t next = tail + 1;

            size_t head = m_head.load(std::memory_order_acquire);
            if (tail - head >= m_capacity)
                return false;

            if (m_tail.compare_exchange_weak(
                    tail, next,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed))
            {
                break;
            }
        }

        size_t idx = tail & m_mask;
        Slot& slot = m_buffer[idx];

        slot.ptr = item.get();
        slot.ready.store(true, std::memory_order_release);

        item.release();

        return true;
    }

    void dequeueAll(std::vector<std::unique_ptr<T>>& out)
    {
        out.clear();

        size_t head = m_head.load(std::memory_order_relaxed);
        size_t start = head;

        while (true)
        {
            size_t idx = head & m_mask;
            Slot& slot = m_buffer[idx];

            if (!slot.ready.load(std::memory_order_acquire))
                break;

            T* ptr = slot.ptr;

            slot.ptr = nullptr;
            slot.ready.store(false, std::memory_order_release);

            out.emplace_back(ptr);

            ++head;
        }

        if (head != start)
        {
            m_head.store(head, std::memory_order_release);
        }
    }

    bool empty() const
    {
        return m_head.load(std::memory_order_acquire) ==
               m_tail.load(std::memory_order_acquire);
    }

private:
    struct Slot
    {
        std::atomic<bool> ready{false};
        T* ptr{nullptr};
    };

    size_t m_capacity;
    size_t m_mask;

    std::unique_ptr<Slot[]> m_buffer;

    alignas(64) std::atomic<size_t> m_head{0};
    alignas(64) std::atomic<size_t> m_tail{0};
};
