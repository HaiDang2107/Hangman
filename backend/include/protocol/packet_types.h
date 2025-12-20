#ifndef PACKET_TYPES_H
#define PACKET_TYPES_H

#include <cstdint>
#include <cstddef>

namespace hangman {

// Protocol version
constexpr uint8_t PROTOCOL_VERSION = 1;

// Maxs
constexpr size_t MAX_USERNAME_LEN = 64;
constexpr size_t MAX_PASSWORD_LEN = 64;
constexpr size_t MAX_TOKEN_LEN = 256;
constexpr size_t MAX_MESSAGE_LEN = 1024;
constexpr size_t MAX_ROOM_NAME_LEN = 128;

// Packet types (u16)
enum class PacketType : uint16_t {
    // Authentication
    C2S_Register           = 0x0101,
    S2C_RegisterResult     = 0x0102,
    C2S_Login              = 0x0103,
    S2C_LoginResult        = 0x0104,
    C2S_Logout             = 0x0105,
    S2C_LogoutAck          = 0x0106,

    // Lobby / Room
    C2S_CreateRoom         = 0x0201,
    S2C_CreateRoomResult   = 0x0202,
    C2S_LeaveRoom          = 0x0203,
    S2C_LeaveRoomAck       = 0x0204,
    S2C_PlayerLeftNotification = 0x0205,
    C2S_RequestOnlineList  = 0x0206,
    S2C_OnlineList         = 0x0207,
    C2S_KickPlayer         = 0x0208,
    S2C_KickResult         = 0x0209,

    // Invite / Match
    C2S_SendInvite         = 0x0301,
    S2C_InviteReceived     = 0x0302,
    C2S_RespondInvite      = 0x0303,
    S2C_InviteResponse     = 0x0304,

    // Ready / Start
    C2S_SetReady           = 0x0401,
    S2C_PlayerReadyUpdate  = 0x0402,
    C2S_StartGame          = 0x0403,
    S2C_GameStart          = 0x0404,
    C2S_RespondInvite      = 0x0303, // accept/reject
    S2C_InviteResponse     = 0x0304,

    // Ready / Game start
    C2S_SetReady           = 0x0401,
    S2C_PlayerReadyUpdate  = 0x0402,
    S2C_GameStart          = 0x0403,

    // Game actions
    C2S_GuessChar          = 0x0501,
    S2C_GuessCharResult    = 0x0502,
    C2S_GuessWord          = 0x0503,
    S2C_GuessWordResult    = 0x0504,
    C2S_RequestDraw        = 0x0505,
    S2C_DrawRequest        = 0x0506,
    C2S_EndGame            = 0x0507,
    S2C_GameEnd            = 0x0508,

    // Records / leaderboard
    C2S_RequestHistory     = 0x0601,
    S2C_HistoryList        = 0x0602,
    C2S_RequestLeaderboard = 0x0603,
    S2C_Leaderboard        = 0x0604,

    // Generic ack / error
    S2C_Ack                = 0x0FFF,
    S2C_Error              = 0x0FFE
};

enum class ResultCode : uint8_t {
    OK = 0,
    FAIL = 1,
    AUTH_FAIL = 2,
    INVALID = 3,
    NOT_FOUND = 4,
    ALREADY = 5,
    SERVER_ERROR = 6
};

} // namespace hangman

#endif // PACKET_TYPES_H
