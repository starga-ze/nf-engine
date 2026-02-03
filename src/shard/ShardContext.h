#pragma once

#include "db/DbManager.h"

#include <memory>

class ShardManager;
class TxRouter;
class DbManager;

class LoginContext;
class LobbyContext;
class MarketContext;
class PingContext;

class ShardContext {
public:
    ShardContext(int shardIdx, uint8_t marketId, ShardManager *shardManager, DbManager *dbManager);

    ~ShardContext();

    ShardManager& shardManager();
    LoginContext& loginContext();
    LobbyContext& lobbyContext();
    MarketContext& marketContext();
    PingContext& pingContext();

    void setTxRouter(TxRouter *txRouter);

private:
    ShardManager* m_shardManager;
    DbManager* m_dbManager;

    std::unique_ptr <LoginContext> m_loginContext;
    std::unique_ptr <LobbyContext> m_lobbyContext;
    std::unique_ptr <MarketContext> m_marketContext;
    std::unique_ptr <PingContext> m_pingContext;

    int m_shardIdx;
    uint8_t m_marketId;
};

