#include "LobbyParser.h"
#include "util/Logger.h"
#include "execution/lobby/LobbyEvent.h"

#include <cstring>

std::unique_ptr<Event> LobbyParser::deserialize(ParsedPacket& parsed)
{

    switch(parsed.opcode())
    {
        case Opcode::LOBBY_ENTRY_REQ:
            return parseLobbyEntryReq(parsed);
        default:
            return nullptr;
    }
}

std::unique_ptr<Event> LobbyParser::parseLobbyEntryReq(ParsedPacket& parsed)
{
    auto payload = parsed.takePayload();

    constexpr size_t HEADER_SIZE = 16;
    const size_t bodyLen = parsed.bodySize();

    if (bodyLen != 0)
    {
        LOG_WARN("LOBBY_ENTRY_REQ unexpected body: payload={}, bodyLen={}", payload.size(), bodyLen);
        return nullptr;
    }

    if (payload.size() < HEADER_SIZE)
    {
        LOG_WARN("LOBBY_ENTRY_REQ invalid payload size: payload={}", payload.size());
        return nullptr;
    }

    return std::make_unique<LobbyEntryReqEvent>(parsed.getSessionId(), std::move(payload));
}
