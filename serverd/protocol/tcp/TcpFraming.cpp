#include "protocol/tcp/TcpFraming.h"
#include "packet/ParsedPacketTypes.h"

#include <arpa/inet.h>
#include <cstring>

#define TCP_HEADER_SIZE   (sizeof(CommonPacketHeader))
#define TCP_MAX_BODY_LEN  (64 * 1024)

TcpFramingResult
TcpFraming::tryExtractFrame(const ByteRingBuffer& rxRing, size_t& outFrameLen)
{
    if (rxRing.readable() < TCP_HEADER_SIZE)
    {
        return TcpFramingResult::NeedMoreData;
    }

    uint8_t headerBuf[TCP_HEADER_SIZE];
    rxRing.peek(headerBuf, TCP_HEADER_SIZE);

    CommonPacketHeader hdr{};
    std::memcpy(&hdr, headerBuf, TCP_HEADER_SIZE);

    uint16_t bodyLen = ntohs(hdr.bodyLen);

    if (bodyLen > TCP_MAX_BODY_LEN)
    {
        return TcpFramingResult::InvalidBodyLen;
    }

    size_t frameLen = TCP_HEADER_SIZE + bodyLen;

    if (rxRing.readable() < frameLen)
    {
        return TcpFramingResult::NeedMoreData;
    }

    outFrameLen = frameLen;
    return TcpFramingResult::Ok;
}

