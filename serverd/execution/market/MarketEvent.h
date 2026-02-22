#pragma once

#include "core/CoreControlTypes.h"
#include "execution/Event.h"

#include <future>

class MarketEvent : public ExternalEvent
{
    protected:
        MarketEvent(uint64_t sessionId);
};

class MarketDumpReqEvent : public InternalEvent
{
public:
    MarketDumpReqEvent(std::promise<MarketInfo>);

    void handleEvent(ShardContext& ctx) override;
    void fulfill(MarketInfo marketInfo);

private:
    std::promise<MarketInfo> m_marketInfo;
};
