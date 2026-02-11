#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <cstddef>

class Packet;

class MpscQueue
{
public:
    explicit MpscQueue(size_t capacity);
    ~MpscQueue();

    bool enqueue(std::unique_ptr<Packet> item);
    void dequeueAll(std::vector<std::unique_ptr<Packet>>& out);

    bool empty() const;

private:
    struct Slot
    {
        std::atomic<bool> ready;
        Packet* ptr;
    };

    const size_t m_capacity;
    const size_t m_mask;

    std::unique_ptr<Slot[]> m_buffer;

    std::atomic<size_t> m_tail{0}; // producer index allocator
    size_t m_head{0};              // consumer only
};

