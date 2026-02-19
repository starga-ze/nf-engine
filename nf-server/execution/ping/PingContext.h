#pragma once

#include "egress/TxRouter.h"
#include "packet/ParsedPacket.h"

#include "execution/ping/PingEvent.h"
#include "execution/ping/PingAction.h"

#include <memory>
#include <cstdint>

class ShardManager;
class PingReqEvent;
class PingResAction;

class PingContext
{
public:
    PingContext(int shardIdx, ShardManager* shardManager);
    ~PingContext() = default;

    void pingReqEvent(const PingReqEvent &ev);
    void pingResAction(PingResAction& ac);

    void setTxRouter(TxRouter *txRouter);

    uint64_t nonce() const;
    uint64_t clientTs() const;

private:
    ShardManager* m_shardManager;
    TxRouter* m_txRouter;

    int m_shardIdx;
};
