#include "SpscQueue.h"
#include "packet/Packet.h"
#include "util/Logger.h"

#include <stdexcept>

SpscQueue::SpscQueue(size_t capacity)
    : m_capacity(capacity),
      m_mask(capacity - 1),
      m_buffer(std::make_unique<Slot[]>(capacity))
{
    if (capacity == 0 or (capacity & (capacity - 1)) != 0)
    {
        LOG_FATAL("SpscQueue capacity must be power of 2");
        std::abort();
    }

    for (size_t i = 0; i < capacity; ++i)
    {
        m_buffer[i].ptr = nullptr;
    }
}

SpscQueue::~SpscQueue()
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
    LOG_DEBUG("SpscQueue destroy, leak count: {}", leaked);
}

bool SpscQueue::enqueue(std::unique_ptr<Packet> pkt)
{
    size_t tail = m_tail.load(std::memory_order_relaxed);
    size_t next = tail + 1;

    size_t head = m_head.load(std::memory_order_acquire);

    if (next - head > m_capacity)
    {
        return false;
    }

    Slot& slot = m_buffer[tail & m_mask];

    slot.ptr = pkt.get();

    m_tail.store(next, std::memory_order_release);

    pkt.release();

    return true;
}

std::unique_ptr<Packet> SpscQueue::dequeue()
{
    size_t head = m_head.load(std::memory_order_relaxed);
    size_t tail = m_tail.load(std::memory_order_acquire);

    if (head == tail)
    {
        return nullptr;
    }

    Slot& slot = m_buffer[head & m_mask];

    Packet* ptr = slot.ptr;
    slot.ptr = nullptr;

    m_head.store(head + 1, std::memory_order_release);

    return std::unique_ptr<Packet>(ptr);
}

bool SpscQueue::empty() const
{
    return m_head.load(std::memory_order_acquire) ==
           m_tail.load(std::memory_order_acquire);
}

