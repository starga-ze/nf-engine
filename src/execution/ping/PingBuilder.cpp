#include "PingBuilder.h"
#include "util/Logger.h"

#include "shard/ShardContext.h"
#include "shard/ShardManager.h"

#include <cstring>
#include <arpa/inet.h>

std::vector<uint8_t> PingBuilder::serialize(Opcode opcode, uint64_t sessionId,
        uint64_t nonce, uint64_t clientTs)
{
    switch(opcode)
    {
        case Opcode::PING_RES:
            return buildPingRes(opcode, sessionId, nonce, clientTs);

        default:
            return {};
    }
}

std::vector<uint8_t> PingBuilder::buildPingRes(Opcode opcode, uint64_t sessionId, 
        uint64_t nonce, uint64_t clientTs)
{
    constexpr uint8_t  VERSION  = 0x01;
    constexpr uint16_t BODY_LEN = 24; // nonce(8) + clientTs(8) + serverTs(8)

    std::vector<uint8_t> pkt(16 + BODY_LEN);

    // ── Common Header ─────────────────────────────
    pkt[0] = VERSION;
    pkt[1] = static_cast<uint8_t>(opcode);
    pkt[2] = (BODY_LEN >> 8) & 0xFF;
    pkt[3] = BODY_LEN & 0xFF;

    uint64_t sid_be = htobe64(sessionId);
    std::memcpy(pkt.data() + 4, &sid_be, sizeof(sid_be));

    std::memset(pkt.data() + 12, 0, 4); // flags

    // ── Body ─────────────────────────────────────
    size_t off = 16;

    uint64_t nonce_be = htobe64(nonce);
    std::memcpy(pkt.data() + off, &nonce_be, sizeof(nonce_be));
    off += 8;

    uint64_t clientTs_be = htobe64(clientTs);
    std::memcpy(pkt.data() + off, &clientTs_be, sizeof(clientTs_be));
    off += 8;

    uint64_t serverTs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

    uint64_t serverTs_be = htobe64(serverTs);
    std::memcpy(pkt.data() + off, &serverTs_be, sizeof(serverTs_be));

    return pkt;
}
