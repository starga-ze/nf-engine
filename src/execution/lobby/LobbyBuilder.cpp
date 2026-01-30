#include "LobbyBuilder.h"
#include "util/Logger.h"

#include "shard/ShardContext.h"
#include "shard/ShardManager.h"

static constexpr uint8_t VERSION = 0x01;
static constexpr size_t HEAEDER_SIZE = 16;

std::vector<uint8_t> LobbyBuilder::serialize(Opcode opcode, 
        uint64_t sessionId, ShardContext& shardContext)
{
    switch(opcode)
    {
        case Opcode::LOBBY_RES:
            return buildLobbyRes(opcode, sessionId, shardContext);

        default:
            return {};
    }
}

std::vector<uint8_t> LobbyBuilder::buildLobbyRes(Opcode, 
        uint64_t sessionId, ShardContext& shardContext)
{
    const auto& markets = shardContext.shardManager().getMarkets();
    return {};
}
