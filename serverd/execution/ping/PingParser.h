#include "execution/Event.h"
#include "packet/ParsedPacket.h"

#include <memory>

class PingParser
{
public:
    PingParser() = default;
    ~PingParser() = default;

    static std::unique_ptr<Event> deserialize(ParsedPacket& parsed);

private:
    static std::unique_ptr<Event> parsePingReq(ParsedPacket& parsed);
};
