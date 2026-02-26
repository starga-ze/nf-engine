#include "ipc/IpcFraming.h"
#include "algorithm/ByteRingBuffer.h"

#include <cstdint>

#define IPC_HEADER_SIZE   (2)
#define IPC_MAX_BODY_LEN  (64 * 1024)

IpcFramingResult
IpcFraming::tryExtractFrame(const ByteRingBuffer& rxRing,
                            size_t& outFrameLen)
{
    if (rxRing.readable() < IPC_HEADER_SIZE)
    {
        return IpcFramingResult::NeedMoreData;
    }

    uint8_t hdr[IPC_HEADER_SIZE];
    rxRing.peek(hdr, IPC_HEADER_SIZE);

    uint16_t bodyLen =
        (static_cast<uint16_t>(hdr[0]) << 8) |
        static_cast<uint16_t>(hdr[1]);

    if (bodyLen > IPC_MAX_BODY_LEN)
    {
        return IpcFramingResult::InvalidBodyLen;
    }

    size_t frameLen = static_cast<size_t>(bodyLen) + IPC_HEADER_SIZE;

    if (rxRing.readable() < frameLen)
    {
        return IpcFramingResult::NeedMoreData;
    }

    outFrameLen = frameLen;
    return IpcFramingResult::Ok;
}
