#include "algorithm/MpscQueue.h"
#include "packet/Packet.h"
#include "util/Logger.h"

#include <thread>

MpscQueue::MpscQueue(size_t capacity)
    : m_capacity(capacity)
    , m_mask(capacity - 1)
{
    assert(capacity != 0 and (capacity & (capacity - 1)) == 0);

    m_buffer = std::make_unique<std::atomic<Packet*>[]>(capacity);

    for (size_t i = 0; i < capacity; ++i)
    {
        m_buffer[i].store(nullptr, std::memory_order_relaxed);
    }

    m_tail.store(0, std::memory_order_relaxed);
    m_head.store(0, std::memory_order_relaxed);
}

MpscQueue::~MpscQueue()
{
    size_t droppedCount = 0;

    for (size_t i = 0; i < m_capacity; ++i)
    {
        Packet* ptr = m_buffer[i].load(std::memory_order_relaxed);
        if (ptr != nullptr)
        {
            delete ptr;
            droppedCount++;
        }
    }

    if (droppedCount > 0)
    {
        LOG_WARN("MpscQueue destroyed. {} packets remained and were dropped.", droppedCount);
    }
}

bool MpscQueue::enqueue(std::unique_ptr<Packet> item)
{
    size_t tail = m_tail.load(std::memory_order_relaxed);
    size_t head = m_head.load(std::memory_order_acquire);

    if (tail - head >= m_capacity)
    {
        return false;
    }

    size_t idx = tail & m_mask;

    m_buffer[idx].store(item.get(), std::memory_order_release);

    m_tail.store(tail + 1, std::memory_order_release);
    
    item.release();
    return true;
}

void MpscQueue::dequeueAll(std::vector<std::unique_ptr<Packet>>& out)
{
    out.clear();

    size_t head = m_head.load(std::memory_order_relaxed);
    size_t tail = m_tail.load(std::memory_order_acquire);

    while (head < tail)
    {
        size_t idx = head & m_mask;

        Packet* ptr = m_buffer[idx].load(std::memory_order_acquire);
        if (!ptr)
            break;

        m_buffer[idx].store(nullptr, std::memory_order_release);

        out.emplace_back(ptr);
        ++head;
    }

    m_head.store(head, std::memory_order_release);
}

bool MpscQueue::empty() const
{
    return m_head.load(std::memory_order_acquire) >= m_tail.load(std::memory_order_acquire);
}
