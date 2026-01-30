#include "execution/login/LoginAction.h"
#include "execution/login/LoginContext.h"
#include "execution/login/LoginBuilder.h"

LoginSuccessResAction::LoginSuccessResAction(uint64_t sessionId, Opcode opcode) : 
    m_sessionId(sessionId),
    m_opcode(opcode)
{
}

void LoginSuccessResAction::handleAction(ShardContext &shardContext) {
    m_payload = LoginBuilder::serialize(m_opcode, m_sessionId);
    shardContext.loginContext().loginSuccessResAction(*this);
}

LoginFailResAction::LoginFailResAction(uint64_t sessionId, Opcode opcode) : 
    m_sessionId(sessionId),
    m_opcode(opcode)
{
}

void LoginFailResAction::handleAction(ShardContext &shardContext) {
    m_payload = LoginBuilder::serialize(m_opcode, m_sessionId);
    shardContext.loginContext().loginFailResAction(*this);
}
