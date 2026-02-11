#include "MpscQueue.h"
#include "packet/Packet.h"

#include <thread>
#include <cassert>

MpscQueue::MpscQueue(size_t capacity)
    : m_capacity(capacity),
      m_mask(capacity - 1),
      m_buffer(std::make_unique<Slot[]>(capacity))
{
    assert(capacity != 0 && (capacity & (capacity - 1)) == 0);

    for (size_t i = 0; i < capacity; ++i)
    {
        m_buffer[i].ready.store(false, std::memory_order_relaxed);
        m_buffer[i].ptr = nullptr;
    }
}

MpscQueue::~MpscQueue()
{
    for (size_t i = 0; i < m_capacity; ++i)
    {
        if (m_buffer[i].ptr)
            delete m_buffer[i].ptr;
    }
}

bool MpscQueue::enqueue(std::unique_ptr<Packet> item)
{
    size_t seq = m_tail.fetch_add(1, std::memory_order_relaxed);
    size_t idx = seq & m_mask;

    Slot& slot = m_buffer[idx];

    while (slot.ready.load(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }

    slot.ptr = item.get();

    slot.ready.store(true, std::memory_order_release);

    item.release();
    return true;
}

void MpscQueue::dequeueAll(std::vector<std::unique_ptr<Packet>>& out)
{
    out.clear();

    while (true)
    {
        size_t idx = m_head & m_mask;
        Slot& slot = m_buffer[idx];

        if (!slot.ready.load(std::memory_order_acquire))
            break;

        Packet* ptr = slot.ptr;

        slot.ptr = nullptr;

        slot.ready.store(false, std::memory_order_release);

        out.emplace_back(ptr);

        ++m_head;
    }
}

bool MpscQueue::empty() const
{
    const Slot& slot = m_buffer[m_head & m_mask];
    return !slot.ready.load(std::memory_order_acquire);
}

