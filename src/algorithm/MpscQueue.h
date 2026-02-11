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
    struct alignas(64) Slot
    {
        std::atomic<bool> ready;
        Packet* ptr;
    };

    const size_t m_capacity;
    const size_t m_mask;

    std::unique_ptr<Slot[]> m_buffer;

    alignas(64) std::atomic<size_t> m_tail{0};
    alignas(64) std::atomic<size_t> m_head{0};
};

