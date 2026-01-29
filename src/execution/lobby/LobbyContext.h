#pragma once

#include "egress/TxRouter.h"
#include "packet/ParsedPacket.h"

#include "execution/lobby/LobbyEvent.h"
//#include "execution/lobby/LobbyAction.h"

#include <memory>
#include <cstdint>

class ShardManager;
class LobbyReqEvent;

class LobbyContext
{
public:
    LobbyContext(int shardIdx, ShardManager* shardManager);
    ~LobbyContext() = default;

    void lobbyReqEvent(const LobbyReqEvent& ev);
    void setTxRouter(TxRouter *txRouter);

private:
    ShardManager* m_shardManager;
    TxRouter* m_txRouter;

    int m_shardIdx;
};
