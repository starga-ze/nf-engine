#pragma once

#include <vector>
#include <atomic>
#include <memory>
#include <cassert>

class Packet;

class MpscQueue
{
public:
    explicit MpscQueue(size_t capacity);
    ~MpscQueue();

    MpscQueue(const MpscQueue&) = delete;
    MpscQueue& operator=(const MpscQueue&) = delete;

    bool enqueue(std::unique_ptr<Packet> item);

    void dequeueAll(std::vector<std::unique_ptr<Packet>>& outItems);

    bool empty() const;

    size_t capacity() const { return m_capacity; }

private:
    std::unique_ptr<std::atomic<Packet*>[]> m_buffer;

    size_t m_mask;
    size_t m_capacity;

    alignas(64) std::atomic<size_t> m_tail;
    alignas(64) std::atomic<size_t> m_head;
};
