#pragma once

#include "algorithm/ByteRingBuffer.h"

#include <cstddef>

enum class TlsFramingResult
{
    NeedMoreData,
    InvalidBodyLen,
    Ok
};

class TlsFraming
{
public:
    static TlsFramingResult tryExtractFrame(const ByteRingBuffer& rxRing, size_t& outFrameLen);
};
