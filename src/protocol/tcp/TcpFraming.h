#pragma once

#include "algorithm/ByteRingBuffer.h"

#include <vector>
#include <cstdint>

enum class TcpFramingResult 
{
    NeedMoreData,
    InvalidBodyLen,
    Ok
};

class TcpFraming
{
public:
    static TcpFramingResult tryExtractFrame(const ByteRingBuffer& rxRing, size_t& outFrameLen);
};
