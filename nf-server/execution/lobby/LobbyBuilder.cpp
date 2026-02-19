#include "LobbyBuilder.h"
#include "util/Logger.h"

#include "shard/ShardContext.h"
#include "shard/ShardManager.h"

#include <cstring>
#include <arpa/inet.h>

static constexpr uint8_t VERSION = 0x01;
static constexpr size_t HEADER_SIZE = 16;

static constexpr size_t MARKET_COUNT_SIZE = 2;
static constexpr size_t MARKET_ID_SIZE    = 1;
static constexpr size_t ALIAS_SIZE        = 16;

static constexpr size_t ENTRY_SIZE        = MARKET_ID_SIZE + ALIAS_SIZE;

std::vector<uint8_t>
LobbyBuilder::serialize(Opcode opcode,
                        uint64_t sessionId,
                        ShardContext& shardContext)
{
    switch(opcode)
    {
        case Opcode::LOBBY_ENTRY_RES:
            return buildLobbyRes(opcode, sessionId, shardContext);

        default:
            return {};
    }
}

std::vector<uint8_t>
LobbyBuilder::buildLobbyRes(Opcode opcode,
                           uint64_t sessionId,
                           ShardContext& shardContext)
{
    const auto& markets = shardContext.shardManager().getMarkets();

    uint16_t count = static_cast<uint16_t>(markets.size());

    size_t bodyLen = MARKET_COUNT_SIZE + count * ENTRY_SIZE;

    std::vector<uint8_t> payload;
    payload.resize(HEADER_SIZE + bodyLen);

    payload[0] = VERSION;
    payload[1] = static_cast<uint8_t>(opcode);

    uint16_t netLen = htons(static_cast<uint16_t>(bodyLen));
    std::memcpy(&payload[2], &netLen, sizeof(uint16_t));

    uint64_t netSid = htobe64(sessionId);
    std::memcpy(&payload[4], &netSid, sizeof(uint64_t));

    uint32_t flags = 0;
    uint32_t netFlags = htonl(flags);
    std::memcpy(&payload[12], &netFlags, sizeof(uint32_t));

    size_t offset = HEADER_SIZE;

    uint16_t netCount = htons(count);
    std::memcpy(&payload[offset], &netCount, MARKET_COUNT_SIZE);
    offset += MARKET_COUNT_SIZE;

    for (auto& m : markets)
    {
        payload[offset] = m.id;
        offset += MARKET_ID_SIZE;

        char aliasBuf[ALIAS_SIZE] = {0};
        std::strncpy(aliasBuf, m.alias.c_str(), ALIAS_SIZE - 1);

        std::memcpy(&payload[offset], aliasBuf, ALIAS_SIZE);
        offset += ALIAS_SIZE;
    }
    return payload;
}

