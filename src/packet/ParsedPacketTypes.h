#pragma once

/*
 *               Common Packet Structure (16 bytes)
 *
 * - flags reserved for future use
 * ┌────────┬───────────┬────────────┬──────────────┬──────────┐
 * │ ver(1) │ opcode(1) │ bodyLen(2) │ sessionId(8) │ flags(4) │
 * ├────────┴───────────┴────────────┴──────────────┴──────────┤
 * │                 Body (N bytes = bodyLen)                  │
 * └───────────────────────────────────────────────────────────┘
 *
 *
 *
 *                       LOGIN REQ (0x10)
 *
 *  - initial sessionId MUST be 0
 *
 * ┌────────┬───────────┬────────────┬──────────────┬──────────┐
 * │  ver   │ opcode=10 │ bodyLen    │ sessionId=0  │ flags    │
 * ├────────┴───────────┴────────────┴──────────────┴──────────┤
 * │ idLen(2)  │  idBytes(idLen)                               │
 * ├───────────────────────────────────────────────────────────┤
 * │ pwLen(2)  │  pwBytes(pwLen)                               │
 * └───────────────────────────────────────────────────────────┘
 *
 * Body:
 *   idLen   : uint16
 *   idBytes : char[idLen]
 *   pwLen   : uint16
 *   pwBytes : char[pwLen]
 *
 *
 *
 *                    LOGIN SUCCESS RES (0x11)
 *
 *  - sessionId is delivered via Common Header
 *
 * ┌────────┬───────────┬────────────┬──────────────┬──────────┐
 * │  ver   │ opcode=11 │ bodyLen=1  │ sessionId    │ flags    │
 * ├────────┴───────────┴────────────┴──────────────┴──────────┤
 * │ resultCode(1) = 1                                         │
 * └───────────────────────────────────────────────────────────┘
 *
 * Body:
 *   resultCode : uint8 (1 = success)
 *
 *
 *
 *                     LOGIN FAIL RES (0x12)
 *
 * ┌────────┬───────────┬────────────┬──────────────┬──────────┐
 * │  ver   │ opcode=12 │ bodyLen=1  │ sessionId    │ flags    │
 * ├────────┴───────────┴────────────┴──────────────┴──────────┤
 * │ resultCode(1)=0                                           │
 * └───────────────────────────────────────────────────────────┘
 *
 * Body:
 *   resultCode : uint8  (0 = fail)
 *
 *
 *
 *                    LOBBY ENTRY REQ (0x20)
 *
 *  - requests market/channel list after login
 *  - Body is empty
 *
 * ┌────────┬───────────┬────────────┬──────────────┬──────────┐
 * │  ver   │ opcode=20 │ bodyLen=0  │ sessionId    │ flags    │
 * ├────────┴───────────┴────────────┴──────────────┴──────────┤
 * │                         (empty)                           │
 * └───────────────────────────────────────────────────────────┘
 *
 *
 *
 *                    LOBBY ENTRY RES (0x21)
 *
 *  - responds with available market/channel list
 *
 * ┌────────┬───────────┬────────────┬──────────────┬──────────┐
 * │  ver   │ opcode=21 │ bodyLen    │ sessionId    │ flags    │
 * ├────────┴───────────┴────────────┴──────────────┴──────────┤
 * │ marketCount(2)                                            │
 * ├───────────────────────────────────────────────────────────┤
 * │ marketId(1) │ alias(16 bytes, null padded)                │
 * │ marketId(1) │ alias(16 bytes, null padded)                │
 * │ ... repeated marketCount times ...                        │
 * └───────────────────────────────────────────────────────────┘
 *
 * Body:
 *   marketCount : uint16
 *   entry:
 *     marketId  : uint8
 *     alias     : char[16]
 *
 * BodyLen = 2 + N * (1 + 16)
 *
 * Example (N=2):
 *   [00 02]
 *   [01][M1\0..........]
 *   [02][M2\0..........]
 *
 *
 *
 *                       PING REQ (0x90)
 *
 * ┌────────┬───────────┬────────────┬──────────────┬──────────┐
 * │  ver   │ opcode=90 │ bodyLen=16 │ sessionId    │ flags    │
 * ├────────┴───────────┴────────────┴──────────────┴──────────┤
 * │ nonce(8)                                                  │
 * ├───────────────────────────────────────────────────────────┤
 * │ clientTs(8)                                               │
 * └───────────────────────────────────────────────────────────┘
 *
 * Body:
 *   nonce    : uint64
 *   clientTs : uint64
 *
 *
 *
 *                       PING RES (0x91)
 *                       
 * ┌────────┬───────────┬────────────┬──────────────┬──────────┐
 * │  ver   │ opcode=91 │ bodyLen=24 │ sessionId    │ flags    │
 * ├────────┴───────────┴────────────┴──────────────┴──────────┤
 * │ nonce(8)                                                  │
 * ├───────────────────────────────────────────────────────────┤
 * │ clientTs(8)                                               │
 * ├───────────────────────────────────────────────────────────┤
 * │ serverTs(8)                                               │
 * └───────────────────────────────────────────────────────────┘
 *
 * Body:
 *   nonce    : uint64
 *   clientTs : uint64
 *   serverTs : uint64
 */



#include "packet/Packet.h"

enum class PacketVersion : uint8_t {
    V1 = 1,
};

enum class Opcode : uint8_t {
    LOGIN_REQ = 0x10,           // 16
    LOGIN_SUCCESS_RES = 0x11,   // 17
    LOGIN_FAIL_RES = 0x12,

    LOBBY_ENTRY_REQ = 0x20,     // 32
    LOBBY_ENTRY_RES = 0x21,

    PING_REQ = 0x90,            // 144
    PING_RES = 0x91,            // 145

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
