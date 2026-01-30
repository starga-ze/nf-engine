#include "egress/ActionFactory.h"
#include "util/Logger.h"

#include "execution/login/LoginAction.h"
#include "execution/lobby/LobbyAction.h"

std::unique_ptr <Action> ActionFactory::create(Opcode opcode, uint64_t sessionId) {

    std::unique_ptr<Action> action;

    switch (opcode) {
        case Opcode::LOGIN_SUCCESS_RES:
            action = std::make_unique<LoginSuccessResAction>(sessionId, opcode);
            return action;

        case Opcode::LOGIN_FAIL_RES:
            action = std::make_unique<LoginFailResAction>(sessionId, opcode);
            return action;

        case Opcode::LOBBY_ENTRY_RES:
            action = std::make_unique<LobbyEntryResAction>(sessionId, opcode);
            return action;

        default:
            LOG_WARN("Unhandled opcode {}", static_cast<int>(opcode));
            return nullptr;
    }
}
