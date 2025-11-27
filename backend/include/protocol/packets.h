#ifndef PACKETS_H
#define PACKETS_H

#include "packet_types.h"
#include "bytebuffer.h"
#include <vector>
#include <optional>

namespace hangman {

// Packet header
struct PacketHeader {
    uint8_t version;
    PacketType type;
    uint32_t payload_len;

    static constexpr size_t HEADER_SIZE = 1 + 2 + 4;

    // serialize header (not payload)
    static std::vector<uint8_t> encode_header(uint8_t version, PacketType type, uint32_t payload_len) {
        ByteBuffer bb;
        bb.write_u8(version);
        bb.write_u16(static_cast<uint16_t>(type));
        bb.write_u32(payload_len);
        return bb.buf;
    }

    // parse header from buffer begin; throws if insufficient
    static PacketHeader parse_header(const uint8_t* data, size_t len);
};

// --- Packets definitions ---
// Each packet has:
//  - to_bytes(): full packet bytes (header+payload)
//  - static parse_payload(ByteBuffer&) to parse only payload (header already read)

// Authentication
struct C2S_Register {
    std::string username;
    std::string password; // should be hashed on client ideally
    std::vector<uint8_t> to_bytes() const;
    static C2S_Register from_payload(ByteBuffer& bb);
};

struct S2C_RegisterResult {
    ResultCode code;
    std::string message;
    std::vector<uint8_t> to_bytes() const;
    static S2C_RegisterResult from_payload(ByteBuffer& bb);
};

struct C2S_Login {
    std::string username;
    std::string password;
    std::vector<uint8_t> to_bytes() const;
    static C2S_Login from_payload(ByteBuffer& bb);
};

struct S2C_LoginResult {
    ResultCode code;
    std::string message;
    std::string session_token; // if OK
    std::vector<uint8_t> to_bytes() const;
    static S2C_LoginResult from_payload(ByteBuffer& bb);
};

struct C2S_Logout {
    std::string session_token;
    std::vector<uint8_t> to_bytes() const;
    static C2S_Logout from_payload(ByteBuffer& bb);
};

struct S2C_LogoutAck {
    ResultCode code;
    std::string message;
    std::vector<uint8_t> to_bytes() const;
    static S2C_LogoutAck from_payload(ByteBuffer& bb);
};

// Lobby
struct C2S_CreateRoom {
    std::string session_token;
    std::string room_name;
    std::vector<uint8_t> to_bytes() const;
    static C2S_CreateRoom from_payload(ByteBuffer& bb);
};

struct S2C_CreateRoomResult {
    ResultCode code;
    std::string message;
    uint64_t room_id; // 0 means none
    std::vector<uint8_t> to_bytes() const;
    static S2C_CreateRoomResult from_payload(ByteBuffer& bb);
};

struct C2S_LeaveRoom {
    std::string session_token;
    uint64_t room_id;
    std::vector<uint8_t> to_bytes() const;
    static C2S_LeaveRoom from_payload(ByteBuffer& bb);
};

struct S2C_LeaveRoomAck {
    ResultCode code;
    std::string message;
    std::vector<uint8_t> to_bytes() const;
    static S2C_LeaveRoomAck from_payload(ByteBuffer& bb);
};

struct C2S_RequestOnlineList {
    std::string session_token;
    std::vector<uint8_t> to_bytes() const;
    static C2S_RequestOnlineList from_payload(ByteBuffer& bb);
};

struct S2C_OnlineList {
    // sequence of usernames
    std::vector<std::string> users;
    std::vector<uint8_t> to_bytes() const;
    static S2C_OnlineList from_payload(ByteBuffer& bb);
};

// Invite / match
struct C2S_SendInvite {
    std::string session_token;
    std::string target_username;
    std::vector<uint8_t> to_bytes() const;
    static C2S_SendInvite from_payload(ByteBuffer& bb);
};

struct S2C_InviteReceived {
    std::string from_username;
    uint64_t room_id; // where match will occur (or 0)
    std::vector<uint8_t> to_bytes() const;
    static S2C_InviteReceived from_payload(ByteBuffer& bb);
};

struct C2S_RespondInvite {
    std::string session_token;
    std::string from_username;
    bool accept;
    uint64_t room_id;
    std::vector<uint8_t> to_bytes() const;
    static C2S_RespondInvite from_payload(ByteBuffer& bb);
};

struct S2C_InviteResponse {
    std::string to_username;
    bool accepted;
    std::string message;
    std::vector<uint8_t> to_bytes() const;
    static S2C_InviteResponse from_payload(ByteBuffer& bb);
};

// Ready / start
struct C2S_SetReady {
    std::string session_token;
    uint64_t room_id;
    bool ready;
    std::vector<uint8_t> to_bytes() const;
    static C2S_SetReady from_payload(ByteBuffer& bb);
};

struct S2C_PlayerReadyUpdate {
    std::string username;
    bool ready;
    std::vector<uint8_t> to_bytes() const;
    static S2C_PlayerReadyUpdate from_payload(ByteBuffer& bb);
};

struct S2C_GameStart {
    uint64_t room_id;
    std::string opponent_username;
    uint32_t seed; // random seed for word selection (optional)
    std::vector<uint8_t> to_bytes() const;
    static S2C_GameStart from_payload(ByteBuffer& bb);
};

// Game actions
struct C2S_GuessChar {
    std::string session_token;
    uint64_t match_id;
    char ch;
    std::vector<uint8_t> to_bytes() const;
    static C2S_GuessChar from_payload(ByteBuffer& bb);
};

struct S2C_GuessCharResult {
    bool correct;
    std::string exposed_pattern; // e.g. "_ a _ _"
    uint8_t remaining_attempts;
    std::vector<uint8_t> to_bytes() const;
    static S2C_GuessCharResult from_payload(ByteBuffer& bb);
};

struct C2S_GuessWord {
    std::string session_token;
    uint64_t match_id;
    std::string word;
    std::vector<uint8_t> to_bytes() const;
    static C2S_GuessWord from_payload(ByteBuffer& bb);
};

struct S2C_GuessWordResult {
    bool correct;
    std::string message;
    uint8_t remaining_attempts;
    std::vector<uint8_t> to_bytes() const;
    static S2C_GuessWordResult from_payload(ByteBuffer& bb);
};

struct C2S_RequestDraw {
    std::string session_token;
    uint64_t match_id;
    std::vector<uint8_t> to_bytes() const;
    static C2S_RequestDraw from_payload(ByteBuffer& bb);
};

struct S2C_DrawRequest {
    std::string from_username;
    uint64_t match_id;
    std::vector<uint8_t> to_bytes() const;
    static S2C_DrawRequest from_payload(ByteBuffer& bb);
};

struct C2S_EndGame {
    std::string session_token;
    uint64_t match_id;
    uint8_t result_code; // 0 = resignation, 1 = win, 2 = loss, 3 = draw
    std::string message;
    std::vector<uint8_t> to_bytes() const;
    static C2S_EndGame from_payload(ByteBuffer& bb);
};

struct S2C_GameEnd {
    uint64_t match_id;
    uint8_t result_code;
    std::string summary;
    std::vector<uint8_t> to_bytes() const;
    static S2C_GameEnd from_payload(ByteBuffer& bb);
};

// Records / leaderboard
struct C2S_RequestHistory {
    std::string session_token;
    std::vector<uint8_t> to_bytes() const;
    static C2S_RequestHistory from_payload(ByteBuffer& bb);
};

struct S2C_HistoryList {
    // simple entries: match_id, opponent, result_code, timestamp, summary
    struct Entry {
        uint64_t match_id;
        std::string opponent;
        uint8_t result_code;
        uint64_t timestamp;
        std::string summary;
        void write(ByteBuffer& bb) const;
        static Entry read(ByteBuffer& bb);
    };
    std::vector<Entry> entries;
    std::vector<uint8_t> to_bytes() const;
    static S2C_HistoryList from_payload(ByteBuffer& bb);
};

struct C2S_RequestLeaderboard {
    std::string session_token;
    std::vector<uint8_t> to_bytes() const;
    static C2S_RequestLeaderboard from_payload(ByteBuffer& bb);
};

struct S2C_Leaderboard {
    struct Row {
        std::string username;
        uint32_t wins;
        uint32_t losses;
        uint32_t draws;
        void write(ByteBuffer& bb) const;
        static Row read(ByteBuffer& bb);
    };
    std::vector<Row> rows;
    std::vector<uint8_t> to_bytes() const;
    static S2C_Leaderboard from_payload(ByteBuffer& bb);
};

// Generic
struct S2C_Ack {
    uint16_t ack_for_type;
    ResultCode code;
    std::string message;
    std::vector<uint8_t> to_bytes() const;
    static S2C_Ack from_payload(ByteBuffer& bb);
};

struct S2C_Error {
    uint16_t for_type;
    std::string message;
    std::vector<uint8_t> to_bytes() const;
    static S2C_Error from_payload(ByteBuffer& bb);
};

} // namespace hangman

#endif // PACKETS_H
