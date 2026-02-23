#pragma once

#include "algorithm/ByteRingBuffer.h"

#include <cstddef>
#include <cstdint>

enum class IpcFramingResult
{
    Ok,
    NeedMoreData,
    InvalidLength
};

class IpcFraming
{
public:
    static constexpr size_t HEADER_SIZE = sizeof(uint16_t);
    static constexpr size_t MAX_BODY    = 64 * 1024;

    static IpcFramingResult tryExtractFrame(const ByteRingBuffer& rxRing, size_t& outFrameLen);
    static bool peekBodyLen(const ByteRingBuffer& rxRing, uint16_t& outBodyLen);
};
