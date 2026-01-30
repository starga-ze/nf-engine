#include "execution/login/LoginAction.h"
#include "execution/login/LoginContext.h"
#include "execution/login/LoginBuilder.h"

LoginSuccessAction::LoginSuccessAction(uint64_t sessionId, Opcode opcode) : 
    m_sessionId(sessionId),
    m_opcode(opcode)
{
}

void LoginSuccessAction::handleAction(ShardContext &shardContext) {
    m_payload = LoginBuilder::serialize(m_opcode, m_sessionId);
    shardContext.loginContext().loginSuccessAction(*this);
}

LoginFailAction::LoginFailAction(uint64_t sessionId, Opcode opcode) : 
    m_sessionId(sessionId),
    m_opcode(opcode)
{
}

void LoginFailAction::handleAction(ShardContext &shardContext) {
    m_payload = LoginBuilder::serialize(m_opcode, m_sessionId);
    shardContext.loginContext().loginFailAction(*this);
}
