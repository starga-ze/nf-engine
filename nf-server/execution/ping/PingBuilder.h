#include "execution/Action.h"
#include "packet/ParsedPacketTypes.h"

#include <vector>

class PingBuilder
{
public:
    PingBuilder() = default;
    ~PingBuilder() = default;

    static std::vector<uint8_t> serialize(Opcode opcode, 
            uint64_t sessionId, uint64_t nonce, uint64_t clientTs);

private:
    static std::vector<uint8_t> buildPingRes(Opcode opcode, 
            uint64_t sessionId, uint64_t nonce, uint64_t clientTs);
};
