#pragma once

#include <vector>
#include <mutex>
#include <memory>

class Packet;

class MpscQueue
{
public:
    MpscQueue() = default;
    ~MpscQueue() = default;

    void enqueue(std::unique_ptr<Packet> item);
    void dequeueAll(std::vector<std::unique_ptr<Packet>>& outItems);

    bool empty();

private:
    std::mutex m_lock;
    std::vector<std::unique_ptr<Packet>> m_queue;
};
