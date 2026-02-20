#include "protocol/tls/TlsFraming.h"
#include "packet/ParsedPacketTypes.h"

#include <arpa/inet.h>
#include <cstring>

#define TLS_HEADER_SIZE   (sizeof(CommonPacketHeader))
#define TLS_MAX_BODY_LEN  (64 * 1024)

TlsFramingResult
TlsFraming::tryExtractFrame(const ByteRingBuffer& rxRing, size_t& outFrameLen)
{
    if (rxRing.readable() < TLS_HEADER_SIZE)
    {
        return TlsFramingResult::NeedMoreData;
    }

    uint8_t headerBuf[TLS_HEADER_SIZE];
    rxRing.peek(headerBuf, TLS_HEADER_SIZE);

    CommonPacketHeader hdr{};
    std::memcpy(&hdr, headerBuf, TLS_HEADER_SIZE);

    uint16_t bodyLen = ntohs(hdr.bodyLen);

    if (bodyLen > TLS_MAX_BODY_LEN)
    {
        return TlsFramingResult::InvalidBodyLen;
    }

    size_t frameLen = TLS_HEADER_SIZE + bodyLen;

    if (rxRing.readable() < frameLen)
    {
        return TlsFramingResult::NeedMoreData;
    }

    outFrameLen = frameLen;
    return TlsFramingResult::Ok;
}
