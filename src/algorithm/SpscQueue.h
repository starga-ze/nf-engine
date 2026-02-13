#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

class Packet;

class SpscQueue
{
public:
    explicit SpscQueue(size_t capacity);
    ~SpscQueue();

    bool enqueue(std::unique_ptr<Packet> pkt);

    std::unique_ptr<Packet> dequeue();

    bool empty() const;

private:
    struct Slot
    {
        Packet* ptr;
    };

    const size_t m_capacity;
    const size_t m_mask;

    std::unique_ptr<Slot[]> m_buffer;

    alignas(64) std::atomic<size_t> m_head{0};
    alignas(64) std::atomic<size_t> m_tail{0};
};

