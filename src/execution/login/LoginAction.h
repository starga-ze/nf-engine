#include "execution/Action.h"
#include "execution/shard/ShardContext.h"

#include <cstdint>

class LoginAction : public Action {
public:
    LoginAction() = default;
    virtual ~LoginAction() = default;

};


class LoginSuccessAction final : public LoginAction {
public:
    LoginSuccessAction(uint64_t sessionId);

    void handleAction(ShardContext &shardContext) override;

private:
    uint64_t m_sessionId;
};


class LoginFailAction final : public LoginAction {
public:
    LoginFailAction(uint64_t sessionId);

    void handleAction(ShardContext &shardContext) override;

private:
    uint64_t m_sessionId;
};



