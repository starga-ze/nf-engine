#include "TcpFraming.h"
#include "packet/ParsedPacketTypes.h"

#include <cstring>
#include <arpa/inet.h>

#define TCP_HEADER_SIZE        (sizeof(CommonPacketHeader)) // 8 bytes
#define TCP_MAX_BODY_LEN       (64 * 1024)                  // 64 KB


TcpFramingResult TcpFraming::tryExtractFrame(std::vector<uint8_t>& rxBuffer,
                                           std::vector<uint8_t>& payload,
                                           uint16_t& bodyLenOut) 
{
    if (rxBuffer.size() < TCP_HEADER_SIZE)
    {
        return TcpFramingResult::NeedMoreData;
    }

    CommonPacketHeader hdr{};
    std::memcpy(&hdr, rxBuffer.data(), TCP_HEADER_SIZE);

    uint16_t bodyLen = ntohs(hdr.bodyLen);
    bodyLenOut = bodyLen;

    if (bodyLen > TCP_MAX_BODY_LEN)
    {
        return TcpFramingResult::InvalidBodyLen;
    }

    size_t frameLen = TCP_HEADER_SIZE + bodyLen;

    if (rxBuffer.size() < frameLen)
    {
        return TcpFramingResult::NeedMoreData;
    }

    payload.assign(rxBuffer.begin(), rxBuffer.begin() + frameLen);

    rxBuffer.erase(rxBuffer.begin(), rxBuffer.begin() + frameLen);

    return TcpFramingResult::Ok;
}
