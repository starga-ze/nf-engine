#pragma once

/*
 *              <Common Packet Structure (16 bytes)>
 *
 *  Note:
 * ┌────────┬───────────┬────────────┬──────────────┬──────────┐
 * │ ver(1) │ opcode(1) │ bodyLen(2) │ sessionId(8) │ flags(4) │
 * ├────────┴───────────┴────────────┴──────────────┴──────────┤
 * │                 Body (N bytes = bodyLen)                  │
 * └───────────────────────────────────────────────────────────┘
 */



#include "packet/Packet.h"

enum class PacketVersion : uint8_t {
    V1 = 1,
};

enum class Opcode : uint8_t {
    LOGIN_REQ = 0x10,           // 16
    LOGIN_RES_SUCCESS = 0x11,   // 17
    LOGIN_RES_FAIL = 0x12,

    LOGOUT_REQ = 0x15,
    LOGOUT_RES_SUCCESS = 0x16,
    LOGOUT_RES_FAIL = 0x17,

    LOBBY_REQ = 0x20,     // 32
    LOBBY_RES = 0x21,


    INVALID = 0xFF              // 255
};

#pragma pack(push, 1)
struct CommonPacketHeader {
    PacketVersion version;
    Opcode opcode;
    uint16_t bodyLen;
    uint64_t sessionId;
    uint32_t flags;      //reserved / future use
};
#pragma pack(pop)
