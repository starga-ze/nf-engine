#include "execution/Event.h"
#include "packet/ParsedPacket.h"

#include <memory>

class LobbyParser
{
public:
    LobbyParser() = default;
    ~LobbyParser() = default;

    static std::unique_ptr<Event> deserialize(ParsedPacket& parsed);

private:
    static std::unique_ptr<Event> parseLobbyEntryReq(ParsedPacket& packet);
};
