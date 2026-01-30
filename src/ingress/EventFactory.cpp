#include "util/Logger.h"
#include "ingress/EventFactory.h"
#include "packet/ParsedPacket.h"

#include "execution/login/LoginParser.h"
#include "execution/lobby/LobbyParser.h"

std::unique_ptr <Event> EventFactory::create(ParsedPacket &parsed) {

    switch (parsed.opcode()) {
        case Opcode::LOGIN_REQ:
            return LoginParser::deserialize(parsed);

        case Opcode::LOBBY_ENTRY_REQ:
            return LobbyParser::deserialize(parsed);
            return nullptr;

        default:
            LOG_WARN("Unhandled opcode {}", static_cast<int>(parsed.opcode()));
            return nullptr;
    }
}

