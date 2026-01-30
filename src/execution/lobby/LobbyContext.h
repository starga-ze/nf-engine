#pragma once

#include "egress/TxRouter.h"
#include "packet/ParsedPacket.h"

#include "execution/lobby/LobbyEvent.h"
#include "execution/lobby/LobbyAction.h"

#include <memory>
#include <cstdint>

class ShardManager;
class LobbyEntryReqEvent;
class LobbyEntryResAction;

class LobbyContext
{
public:
    LobbyContext(int shardIdx, ShardManager* shardManager);
    ~LobbyContext() = default;

    void lobbyEntryReqEvent(const LobbyEntryReqEvent& ev);
    void lobbyEntryResAction(LobbyEntryResAction& ac);

    void setTxRouter(TxRouter *txRouter);

private:
    ShardManager* m_shardManager;
    TxRouter* m_txRouter;

    int m_shardIdx;
};
