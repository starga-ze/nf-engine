#include "MpscQueue.h"
#include "packet/Packet.h"
#include "util/Logger.h"

#include <thread>
#include <cassert>

MpscQueue::MpscQueue(size_t capacity)
    : m_capacity(capacity),
      m_mask(capacity - 1),
      m_buffer(std::make_unique<Slot[]>(capacity))
{
    if (capacity == 0 or (capacity & (capacity - 1)) != 0)
    {
        LOG_FATAL("MpscQueue capacity must be power of 2");
        std::abort();
    }

    for (size_t i = 0; i < capacity; ++i)
    {
        m_buffer[i].ready.store(false, std::memory_order_relaxed);
        m_buffer[i].ptr = nullptr;
    }
}

MpscQueue::~MpscQueue()
{
    size_t leaked = 0;

    for (size_t i = 0; i < m_capacity; ++i)
    {
        if (m_buffer[i].ptr)
        {
            delete m_buffer[i].ptr;
            ++leaked;
        }
    }

    LOG_DEBUG("MpscQueue destroy, leak count: {}", leaked);
}

bool MpscQueue::enqueue(std::unique_ptr<Packet> item)
{
    size_t tail;

    while (true)
    {
        tail = m_tail.load(std::memory_order_relaxed);

        size_t head = m_head.load(std::memory_order_acquire);
        if (tail - head >= m_capacity)
        {
            return false;
        }

        if (m_tail.compare_exchange_weak(tail, tail + 1, 
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


void MpscQueue::dequeueAll(std::vector<std::unique_ptr<Packet>>& out)
{
    out.clear();

    size_t head = m_head.load(std::memory_order_relaxed);
    size_t start = head;

    while (true)
    {
        size_t idx = head & m_mask;
        Slot& slot = m_buffer[idx];

        if (!slot.ready.load(std::memory_order_acquire))
        {
            break;
        }
        
        Packet* ptr = slot.ptr;

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

bool MpscQueue::empty() const
{
    const Slot& slot = m_buffer[m_head & m_mask];
    return !slot.ready.load(std::memory_order_acquire);
}

