#include "algorithm/MpscQueue.h"
#include "packet/Packet.h"

void MpscQueue::enqueue(std::unique_ptr<Packet> item)
{
    std::lock_guard<std::mutex> lock(m_lock);
    m_queue.emplace_back(std::move(item));
}

void MpscQueue::dequeueAll(std::vector<std::unique_ptr<Packet>>& outItems)
{
    outItems.clear();

    std::lock_guard<std::mutex> lock(m_lock);
    if (m_queue.empty())
    {
        return;
    }

    m_queue.swap(outItems);
}

bool MpscQueue::empty()
{
    std::lock_guard<std::mutex> lock(m_lock);
    return m_queue.empty();
}
