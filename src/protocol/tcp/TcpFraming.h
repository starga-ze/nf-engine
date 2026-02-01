#pragma once

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
    static TcpFramingResult tryExtractFrame(std::vector<uint8_t>& rxBuffer, 
            std::vector<uint8_t>& payload, uint16_t& bodyLen);
};
