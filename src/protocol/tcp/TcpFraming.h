#pragma once

#include "algorithm/RingBuffer.h"

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
    static TcpFramingResult tryExtractFrame(const RingBuffer& rxRing, size_t& outFrameLen);
};
