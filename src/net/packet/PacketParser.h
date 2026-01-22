#pragma once

#include "net/packet/Packet.h"
#include "net/packet/ParsedPacket.h"

#include <optional>
#include <memory>

class PacketParser {
public:
    PacketParser() = default;

    std::optional <ParsedPacket> parse(std::unique_ptr <Packet> packet) const;
};

