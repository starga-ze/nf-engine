#include "algorithm/RingBuffer.h"
#include <algorithm>
#include <cstring>

RingBuffer::RingBuffer(size_t capacity)
    : m_buffer(capacity)
{
}

size_t RingBuffer::readable() const
{
    return m_size;
}

size_t RingBuffer::writable() const
{
    return m_buffer.size() - m_size;
}

uint8_t* RingBuffer::writePtr()
{
    if (writable() == 0)
        return nullptr;

    return m_buffer.data() + m_tail;
}

size_t RingBuffer::writeLen() const
{
    if (writable() == 0)
        return 0;

    size_t toEnd = m_buffer.size() - m_tail;
    return std::min(toEnd, writable());
}

void RingBuffer::produce(size_t n)
{
    m_tail = (m_tail + n) % m_buffer.size();
    m_size += n;
}

size_t RingBuffer::write(const uint8_t* data, size_t len)
{
    size_t written = 0;
    while (written < len && writable() > 0) 
    {
        size_t n = std::min(writeLen(), len - written);
        std::memcpy(writePtr(), data + written, n);
        produce(n);
        written += n;
    }
    return written;
}

const uint8_t* RingBuffer::readPtr() const
{
    if (readable() == 0)
        return nullptr;

    return m_buffer.data() + m_head;
}

size_t RingBuffer::readLen() const
{
    if (readable() == 0)
        return 0;

    size_t toEnd = m_buffer.size() - m_head;
    return std::min(toEnd, readable());
}

void RingBuffer::consume(size_t n)
{
    m_head = (m_head + n) % m_buffer.size();
    m_size -= n;
}

size_t RingBuffer::read(uint8_t* out, size_t len)
{
    size_t readBytes = 0;
    while (readBytes < len && readable() > 0) 
    {
        size_t n = std::min(readLen(), len - readBytes);
        std::memcpy(out + readBytes, readPtr(), n);
        consume(n);
        readBytes += n;
    }
    return readBytes;
}

void RingBuffer::clear()
{
    m_head = 0;
    m_tail = 0;
    m_size = 0;
}

