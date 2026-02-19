#include "execution/Action.h"
#include "packet/ParsedPacketTypes.h"

#include <vector>

class LobbyBuilder
{
public:
    LobbyBuilder() = default;
    ~LobbyBuilder() = default;

    static std::vector<uint8_t> serialize(Opcode opcode, 
            uint64_t sessionId, ShardContext& shardContext);

private:
    static std::vector<uint8_t> buildLobbyRes(Opcode opcode, 
            uint64_t sessionId, ShardContext& shardContext);
};
