#pragma once

#include <cstddef>

class ByteRingBuffer;

enum class IpcFramingResult
{
    Ok,
    NeedMoreData,
    InvalidBodyLen
};

class IpcFraming
{
public:
    static IpcFramingResult
    tryExtractFrame(const ByteRingBuffer& rxRing, size_t& outFrameLen);
};
