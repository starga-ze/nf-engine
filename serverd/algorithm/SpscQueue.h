#pragma once

#include <atomic>
#include <memory>
#include <cstddef>
#include <cstdlib>

template<typename T>
class SpscQueue
{
public:
    explicit SpscQueue(size_t capacity)
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
            m_buffer[i].ptr = nullptr;
        }
    }

    ~SpscQueue()
    {
        for (size_t i = 0; i < m_capacity; ++i)
        {
            delete m_buffer[i].ptr;
        }
    }

public:

    bool enqueue(std::unique_ptr<T> pkt)
    {
        size_t tail = m_tail.load(std::memory_order_relaxed);
        size_t next = tail + 1;

        size_t head = m_head.load(std::memory_order_acquire);

        if (next - head > m_capacity)
            return false;

        Slot& slot = m_buffer[tail & m_mask];

        slot.ptr = pkt.get();

        m_tail.store(next, std::memory_order_release);

        pkt.release();

        return true;
    }

    std::unique_ptr<T> dequeue()
    {
        size_t head = m_head.load(std::memory_order_relaxed);
        size_t tail = m_tail.load(std::memory_order_acquire);

        if (head == tail)
            return nullptr;

        Slot& slot = m_buffer[head & m_mask];

        T* ptr = slot.ptr;
        slot.ptr = nullptr;

        m_head.store(head + 1, std::memory_order_release);

        return std::unique_ptr<T>(ptr);
    }

    bool empty() const
    {
        return m_head.load(std::memory_order_acquire) ==
               m_tail.load(std::memory_order_acquire);
    }

private:
    struct Slot
    {
        T* ptr{nullptr};
    };

    size_t m_capacity;
    size_t m_mask;

    std::unique_ptr<Slot[]> m_buffer;

    alignas(64) std::atomic<size_t> m_head{0};
    alignas(64) std::atomic<size_t> m_tail{0};
};
