#include "execution/login/LoginAction.h"
#include "execution/login/LoginContext.h"
#include "execution/login/LoginBuilder.h"

LoginAction::LoginAction(uint64_t sessionId, Opcode opcode) :
    Action(sessionId, opcode)
{

}

LoginSuccessResAction::LoginSuccessResAction(uint64_t sessionId, Opcode opcode) : 
    LoginAction(sessionId, opcode)
{
}

void LoginSuccessResAction::handleAction(ShardContext &shardContext) {
    m_payload = LoginBuilder::serialize(opcode(), sessionId());
    shardContext.loginContext().loginSuccessResAction(*this);
}

LoginFailResAction::LoginFailResAction(uint64_t sessionId, Opcode opcode) : 
    LoginAction(sessionId, opcode)
{
}

void LoginFailResAction::handleAction(ShardContext &shardContext) {
    m_payload = LoginBuilder::serialize(opcode(), sessionId());
    shardContext.loginContext().loginFailResAction(*this);
}
