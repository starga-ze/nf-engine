#include "shard/ShardContext.h"

#include "shard/ShardManager.h"

#include "execution/login/LoginContext.h"
#include "execution/lobby/LobbyContext.h"
#include "execution/market/MarketContext.h"


ShardContext::ShardContext(int shardIdx, ShardManager *shardManager, DbManager *dbManager)
        : m_shardIdx(shardIdx),
          m_shardManager(shardManager),
          m_dbManager(dbManager) {
    m_loginContext = std::make_unique<LoginContext>(shardIdx, shardManager, dbManager);
    m_lobbyContext = std::make_unique<LobbyContext>(shardIdx, shardManager);
    m_marketContext = std::make_unique<MarketContext>(shardIdx);
}

ShardContext::~ShardContext() {
}

void ShardContext::setTxRouter(TxRouter *txRouter) {
    m_loginContext->setTxRouter(txRouter);
    m_lobbyContext->setTxRouter(txRouter);
}

LoginContext& ShardContext::loginContext() {
    return *m_loginContext;
}

LobbyContext& ShardContext::lobbyContext() {
    return *m_lobbyContext;
}

MarketContext& ShardContext::marketContext() {
    return *m_marketContext;
}
