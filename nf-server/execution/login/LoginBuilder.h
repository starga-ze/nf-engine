#include "execution/Action.h"
#include "packet/ParsedPacketTypes.h"

#include <vector>

class LoginBuilder
{
public:
    LoginBuilder() = default;
    ~LoginBuilder() = default;

    static std::vector<uint8_t> serialize(Opcode opcode, uint64_t sessionId);

private:
    static std::vector<uint8_t> buildLoginResSuccess(Opcode opcode, uint64_t sessionId);
    static std::vector<uint8_t> buildLoginResFail(Opcode opcode, uint64_t sessionId);
};
