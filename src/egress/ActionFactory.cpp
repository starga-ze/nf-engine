#include "egress/ActionFactory.h"
#include "util/Logger.h"

#include "execution/login/LoginAction.h"
#include "execution/lobby/LobbyAction.h"

std::unique_ptr <Action> ActionFactory::create(Opcode opcode, uint64_t sessionId) {

    std::unique_ptr<Action> action;

    switch (opcode) {
        case Opcode::LOGIN_RES_SUCCESS:
            action = std::make_unique<LoginSuccessAction>(sessionId, opcode);
            return action;

        case Opcode::LOGIN_RES_FAIL:
            action = std::make_unique<LoginFailAction>(sessionId, opcode);
            return action;

        case Opcode::LOBBY_RES:
            action = std::make_unique<LobbyEntryAction>(sessionId, opcode);
            return action;

        default:
            LOG_WARN("Unhandled opcode {}", static_cast<int>(opcode));
            return nullptr;
    }
}
