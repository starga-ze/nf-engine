#include "PingParser.h"
#include "util/Logger.h"
#include "execution/ping/PingEvent.h"

#include <cstring>

std::unique_ptr<Event> PingParser::deserialize(ParsedPacket& parsed)
{
    switch(parsed.opcode())
    {
        case Opcode::PING_REQ:
            return parsePingReq(parsed);
        default:
            return nullptr;
    }
}

std::unique_ptr<Event> PingParser::parsePingReq(ParsedPacket& parsed)
{
    auto payload = parsed.takePayload();

    constexpr size_t HEADER_SIZE = 16;
    constexpr size_t BODY_SIZE = 16;

    const size_t bodyLen = parsed.bodySize();

    if (payload.size() < HEADER_SIZE or payload.size() < HEADER_SIZE + bodyLen)
    {
        LOG_WARN("PING_REQ invalid payload size: payload={}", payload.size());
        return nullptr;
    }

    if (bodyLen != BODY_SIZE)
    {
        LOG_WARN("PING_REQ invalid body size: expected:{}, actual:{}", BODY_SIZE, bodyLen);
        return nullptr;
    }

    const uint8_t* buf = payload.data() + HEADER_SIZE;
    size_t offset = 0;

    uint64_t nonce;
    std::memcpy(&nonce, buf + offset, sizeof(uint64_t));
    nonce = be64toh(nonce);
    offset += sizeof(uint64_t);

    uint64_t clientTs;
    std::memcpy(&clientTs, buf + offset, sizeof(uint64_t));
    clientTs += be64toh(clientTs);

    return std::make_unique<PingReqEvent>(parsed.getSessionId(), std::move(payload), nonce, clientTs);
}
