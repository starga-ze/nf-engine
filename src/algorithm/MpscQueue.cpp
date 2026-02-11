#include "algorithm/MpscQueue.h"
#include "packet/Packet.h"
#include "util/Logger.h"

#include <thread>

MpscQueue::MpscQueue(size_t capacity)
    : m_capacity(capacity)
    , m_mask(capacity - 1)
{
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
    const size_t tail = m_tail.load(std::memory_order_relaxed);
    const size_t head = m_head.load(std::memory_order_acquire);

    if ((long)(tail - head) >= (long)m_capacity)
    {
        LOG_WARN("MpscQueue is Full! Dropping packet. Head={}, Tail={}, Cap={}", 
                 head, tail, m_capacity);
        return false;
    }

    size_t seq = m_tail.fetch_add(1, std::memory_order_acq_rel);
    size_t idx = seq & m_mask;

    std::atomic<Packet*>& slot = m_buffer[idx];
    Packet* expected = nullptr;

    while (!slot.compare_exchange_weak(expected, item.get(), 
                                       std::memory_order_release, 
                                       std::memory_order_relaxed))
    {
        expected = nullptr;
        std::this_thread::yield(); 
    }

    item.release();
    return true;
}

void MpscQueue::dequeueAll(std::vector<std::unique_ptr<Packet>>& outItems)
{
    outItems.clear();

    while (true)
    {
        size_t head = m_head.load(std::memory_order_relaxed);
        size_t tail = m_tail.load(std::memory_order_acquire);

        if (head >= tail)
        {
            return;
        }

        size_t idx = head & m_mask;
        std::atomic<Packet*>& slot = m_buffer[idx];

        Packet* ptr = slot.load(std::memory_order_acquire);
        
        if (ptr == nullptr)
        {
            std::this_thread::yield();
            continue;
        }

        slot.store(nullptr, std::memory_order_release);
        
        outItems.emplace_back(std::unique_ptr<Packet>(ptr));

        m_head.store(head + 1, std::memory_order_release);
    }
}

bool MpscQueue::empty() const
{
    return m_head.load(std::memory_order_acquire) >= m_tail.load(std::memory_order_acquire);
}
