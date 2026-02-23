#include "ipc/IpcFraming.h"

#include <arpa/inet.h>
#include <cstring>

bool IpcFraming::peekBodyLen(const ByteRingBuffer& rxRing, uint16_t& outBodyLen)
{
    if (rxRing.readable() < HEADER_SIZE)
        return false;

    uint8_t hdrBuf[HEADER_SIZE];
    rxRing.peek(hdrBuf, HEADER_SIZE);

    uint16_t nlen = 0;
    std::memcpy(&nlen, hdrBuf, HEADER_SIZE);
    outBodyLen = ntohs(nlen);
    return true;
}

IpcFramingResult IpcFraming::tryExtractFrame(const ByteRingBuffer& rxRing, size_t& outFrameLen)
{
    uint16_t bodyLen = 0;
    if (!peekBodyLen(rxRing, bodyLen))
        return IpcFramingResult::NeedMoreData;

    if (bodyLen > MAX_BODY)
        return IpcFramingResult::InvalidLength;

    const size_t frameLen = HEADER_SIZE + bodyLen;

    if (rxRing.readable() < frameLen)
        return IpcFramingResult::NeedMoreData;

    outFrameLen = frameLen;
    return IpcFramingResult::Ok;
}
