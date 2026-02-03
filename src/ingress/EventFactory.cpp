#include "util/Logger.h"
#include "ingress/EventFactory.h"
#include "packet/ParsedPacket.h"

#include "execution/login/LoginParser.h"
#include "execution/lobby/LobbyParser.h"
#include "execution/ping/PingParser.h"

std::unique_ptr <Event> EventFactory::create(ParsedPacket &parsed) {

    switch (parsed.opcode()) {
        case Opcode::LOGIN_REQ:
            return LoginParser::deserialize(parsed);

        case Opcode::LOBBY_ENTRY_REQ:
            return LobbyParser::deserialize(parsed);

        case Opcode::PING_REQ:
            return PingParser::deserialize(parsed);

        default:
            LOG_WARN("Unhandled opcode {}", static_cast<int>(parsed.opcode()));
            return nullptr;
    }
}

