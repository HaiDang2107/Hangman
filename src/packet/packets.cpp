#include "packets.h"
#include <stdexcept>

namespace hangman {

// Header parse
PacketHeader PacketHeader::parse_header(const uint8_t* data, size_t len) {
    if (len < HEADER_SIZE) throw std::runtime_error("not enough header bytes");
    PacketHeader h;
    h.version = data[0];
    uint16_t t;
    memcpy(&t, data+1, 2);
    t = ntohs(t);
    h.type = static_cast<PacketType>(t);
    uint32_t pl;
    memcpy(&pl, data+3, 4);
    h.payload_len = ntohl(pl);
    return h;
}

// ========== Helpers to produce full packet bytes ==========
static std::vector<uint8_t> make_packet(PacketType type, const std::vector<uint8_t>& payload) {
    auto hdr = PacketHeader::encode_header(PROTOCOL_VERSION, type, (uint32_t)payload.size());
    std::vector<uint8_t> full;
    full.reserve(hdr.size() + payload.size());
    full.insert(full.end(), hdr.begin(), hdr.end());
    full.insert(full.end(), payload.begin(), payload.end());
    return full;
}

// ---------------- Auth packets ----------------
std::vector<uint8_t> C2S_Register::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(username);
    bb.write_string(password);
    return make_packet(PacketType::C2S_Register, bb.buf);
}
C2S_Register C2S_Register::from_payload(ByteBuffer& bb) {
    C2S_Register p;
    p.username = bb.read_string();
    p.password = bb.read_string();
    return p;
}

std::vector<uint8_t> S2C_RegisterResult::to_bytes() const {
    ByteBuffer bb;
    bb.write_u8(static_cast<uint8_t>(code));
    bb.write_string(message);
    return make_packet(PacketType::S2C_RegisterResult, bb.buf);
}
S2C_RegisterResult S2C_RegisterResult::from_payload(ByteBuffer& bb) {
    S2C_RegisterResult r;
    r.code = static_cast<ResultCode>(bb.read_u8());
    r.message = bb.read_string();
    return r;
}

std::vector<uint8_t> C2S_Login::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(username);
    bb.write_string(password);
    return make_packet(PacketType::C2S_Login, bb.buf);
}
C2S_Login C2S_Login::from_payload(ByteBuffer& bb) {
    C2S_Login p;
    p.username = bb.read_string();
    p.password = bb.read_string();
    return p;
}

std::vector<uint8_t> S2C_LoginResult::to_bytes() const {
    ByteBuffer bb;
    bb.write_u8(static_cast<uint8_t>(code));
    bb.write_string(message);
    bb.write_string(session_token);
    return make_packet(PacketType::S2C_LoginResult, bb.buf);
}
S2C_LoginResult S2C_LoginResult::from_payload(ByteBuffer& bb) {
    S2C_LoginResult r;
    r.code = static_cast<ResultCode>(bb.read_u8());
    r.message = bb.read_string();
    r.session_token = bb.read_string();
    return r;
}

std::vector<uint8_t> C2S_Logout::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(session_token);
    return make_packet(PacketType::C2S_Logout, bb.buf);
}
C2S_Logout C2S_Logout::from_payload(ByteBuffer& bb) {
    C2S_Logout p;
    p.session_token = bb.read_string();
    return p;
}

std::vector<uint8_t> S2C_LogoutAck::to_bytes() const {
    ByteBuffer bb;
    bb.write_u8(static_cast<uint8_t>(code));
    bb.write_string(message);
    return make_packet(PacketType::S2C_LogoutAck, bb.buf);
}
S2C_LogoutAck S2C_LogoutAck::from_payload(ByteBuffer& bb) {
    S2C_LogoutAck r;
    r.code = static_cast<ResultCode>(bb.read_u8());
    r.message = bb.read_string();
    return r;
}

// ---------------- Lobby ----------------
std::vector<uint8_t> C2S_CreateRoom::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(session_token);
    bb.write_string(room_name);
    return make_packet(PacketType::C2S_CreateRoom, bb.buf);
}
C2S_CreateRoom C2S_CreateRoom::from_payload(ByteBuffer& bb) {
    C2S_CreateRoom p;
    p.session_token = bb.read_string();
    p.room_name = bb.read_string();
    return p;
}

std::vector<uint8_t> S2C_CreateRoomResult::to_bytes() const {
    ByteBuffer bb;
    bb.write_u8(static_cast<uint8_t>(code));
    bb.write_string(message);
    // room_id as u64 -> write as two u32 (network order)
    uint64_t id = room_id;
    uint32_t hi = (uint32_t)(id >> 32);
    uint32_t lo = (uint32_t)(id & 0xffffffff);
    bb.write_u32(hi);
    bb.write_u32(lo);
    return make_packet(PacketType::S2C_CreateRoomResult, bb.buf);
}
S2C_CreateRoomResult S2C_CreateRoomResult::from_payload(ByteBuffer& bb) {
    S2C_CreateRoomResult r;
    r.code = static_cast<ResultCode>(bb.read_u8());
    r.message = bb.read_string();
    uint32_t hi = bb.read_u32();
    uint32_t lo = bb.read_u32();
    r.room_id = ((uint64_t)hi << 32) | lo;
    return r;
}

std::vector<uint8_t> C2S_LeaveRoom::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(session_token);
    uint32_t hi = (uint32_t)(room_id >> 32);
    uint32_t lo = (uint32_t)(room_id & 0xffffffff);
    bb.write_u32(hi);
    bb.write_u32(lo);
    return make_packet(PacketType::C2S_LeaveRoom, bb.buf);
}
C2S_LeaveRoom C2S_LeaveRoom::from_payload(ByteBuffer& bb) {
    C2S_LeaveRoom p;
    p.session_token = bb.read_string();
    uint32_t hi = bb.read_u32();
    uint32_t lo = bb.read_u32();
    p.room_id = ((uint64_t)hi << 32) | lo;
    return p;
}

std::vector<uint8_t> S2C_LeaveRoomAck::to_bytes() const {
    ByteBuffer bb;
    bb.write_u8(static_cast<uint8_t>(code));
    bb.write_string(message);
    return make_packet(PacketType::S2C_LeaveRoomAck, bb.buf);
}
S2C_LeaveRoomAck S2C_LeaveRoomAck::from_payload(ByteBuffer& bb) {
    S2C_LeaveRoomAck r;
    r.code = static_cast<ResultCode>(bb.read_u8());
    r.message = bb.read_string();
    return r;
}

std::vector<uint8_t> C2S_RequestOnlineList::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(session_token);
    return make_packet(PacketType::C2S_RequestOnlineList, bb.buf);
}
C2S_RequestOnlineList C2S_RequestOnlineList::from_payload(ByteBuffer& bb) {
    C2S_RequestOnlineList p;
    p.session_token = bb.read_string();
    return p;
}

std::vector<uint8_t> S2C_OnlineList::to_bytes() const {
    ByteBuffer bb;
    bb.write_u16((uint16_t)users.size());
    for (auto &u: users) bb.write_string(u);
    return make_packet(PacketType::S2C_OnlineList, bb.buf);
}
S2C_OnlineList S2C_OnlineList::from_payload(ByteBuffer& bb) {
    S2C_OnlineList r;
    uint16_t cnt = bb.read_u16();
    for (int i=0;i<cnt;i++) r.users.push_back(bb.read_string());
    return r;
}

// ---------------- Invite ----------------
std::vector<uint8_t> C2S_SendInvite::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(session_token);
    bb.write_string(target_username);
    return make_packet(PacketType::C2S_SendInvite, bb.buf);
}
C2S_SendInvite C2S_SendInvite::from_payload(ByteBuffer& bb) {
    C2S_SendInvite p;
    p.session_token = bb.read_string();
    p.target_username = bb.read_string();
    return p;
}

std::vector<uint8_t> S2C_InviteReceived::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(from_username);
    uint32_t hi = (uint32_t)(room_id >> 32);
    uint32_t lo = (uint32_t)(room_id & 0xffffffff);
    bb.write_u32(hi);
    bb.write_u32(lo);
    return make_packet(PacketType::S2C_InviteReceived, bb.buf);
}
S2C_InviteReceived S2C_InviteReceived::from_payload(ByteBuffer& bb) {
    S2C_InviteReceived p;
    p.from_username = bb.read_string();
    uint32_t hi = bb.read_u32();
    uint32_t lo = bb.read_u32();
    p.room_id = ((uint64_t)hi<<32) | lo;
    return p;
}

std::vector<uint8_t> C2S_RespondInvite::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(session_token);
    bb.write_string(from_username);
    bb.write_u8(accept ? 1 : 0);
    uint32_t hi = (uint32_t)(room_id >> 32);
    uint32_t lo = (uint32_t)(room_id & 0xffffffff);
    bb.write_u32(hi);
    bb.write_u32(lo);
    return make_packet(PacketType::C2S_RespondInvite, bb.buf);
}
C2S_RespondInvite C2S_RespondInvite::from_payload(ByteBuffer& bb) {
    C2S_RespondInvite p;
    p.session_token = bb.read_string();
    p.from_username = bb.read_string();
    p.accept = bb.read_u8() != 0;
    uint32_t hi = bb.read_u32();
    uint32_t lo = bb.read_u32();
    p.room_id = ((uint64_t)hi<<32) | lo;
    return p;
}

std::vector<uint8_t> S2C_InviteResponse::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(to_username);
    bb.write_u8(accepted ? 1 : 0);
    bb.write_string(message);
    return make_packet(PacketType::S2C_InviteResponse, bb.buf);
}
S2C_InviteResponse S2C_InviteResponse::from_payload(ByteBuffer& bb) {
    S2C_InviteResponse r;
    r.to_username = bb.read_string();
    r.accepted = bb.read_u8() != 0;
    r.message = bb.read_string();
    return r;
}

// ---------------- Ready / start ----------------
std::vector<uint8_t> C2S_SetReady::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(session_token);
    uint32_t hi = (uint32_t)(room_id >> 32); bb.write_u32(hi);
    uint32_t lo = (uint32_t)(room_id & 0xffffffff); bb.write_u32(lo);
    bb.write_u8(ready ? 1 : 0);
    return make_packet(PacketType::C2S_SetReady, bb.buf);
}
C2S_SetReady C2S_SetReady::from_payload(ByteBuffer& bb) {
    C2S_SetReady p;
    p.session_token = bb.read_string();
    uint32_t hi = bb.read_u32();
    uint32_t lo = bb.read_u32();
    p.room_id = ((uint64_t)hi<<32) | lo;
    p.ready = bb.read_u8() != 0;
    return p;
}

std::vector<uint8_t> S2C_PlayerReadyUpdate::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(username);
    bb.write_u8(ready ? 1 : 0);
    return make_packet(PacketType::S2C_PlayerReadyUpdate, bb.buf);
}
S2C_PlayerReadyUpdate S2C_PlayerReadyUpdate::from_payload(ByteBuffer& bb) {
    S2C_PlayerReadyUpdate p;
    p.username = bb.read_string();
    p.ready = bb.read_u8() != 0;
    return p;
}

std::vector<uint8_t> S2C_GameStart::to_bytes() const {
    ByteBuffer bb;
    uint32_t hi = (uint32_t)(room_id >> 32); bb.write_u32(hi);
    uint32_t lo = (uint32_t)(room_id & 0xffffffff); bb.write_u32(lo);
    bb.write_string(opponent_username);
    bb.write_u32(seed);
    return make_packet(PacketType::S2C_GameStart, bb.buf);
}
S2C_GameStart S2C_GameStart::from_payload(ByteBuffer& bb) {
    S2C_GameStart p;
    uint32_t hi = bb.read_u32();
    uint32_t lo = bb.read_u32();
    p.room_id = ((uint64_t)hi<<32) | lo;
    p.opponent_username = bb.read_string();
    p.seed = bb.read_u32();
    return p;
}

// ---------------- Game actions ----------------
std::vector<uint8_t> C2S_GuessChar::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(session_token);
    uint32_t hi = (uint32_t)(match_id >> 32); bb.write_u32(hi);
    uint32_t lo = (uint32_t)(match_id & 0xffffffff); bb.write_u32(lo);
    bb.write_u8((uint8_t)ch);
    return make_packet(PacketType::C2S_GuessChar, bb.buf);
}
C2S_GuessChar C2S_GuessChar::from_payload(ByteBuffer& bb) {
    C2S_GuessChar p;
    p.session_token = bb.read_string();
    uint32_t hi = bb.read_u32(); uint32_t lo = bb.read_u32();
    p.match_id = ((uint64_t)hi<<32) | lo;
    p.ch = (char)bb.read_u8();
    return p;
}

std::vector<uint8_t> S2C_GuessCharResult::to_bytes() const {
    ByteBuffer bb;
    bb.write_u8(correct?1:0);
    bb.write_string(exposed_pattern);
    bb.write_u8(remaining_attempts);
    return make_packet(PacketType::S2C_GuessCharResult, bb.buf);
}
S2C_GuessCharResult S2C_GuessCharResult::from_payload(ByteBuffer& bb) {
    S2C_GuessCharResult r;
    r.correct = bb.read_u8() != 0;
    r.exposed_pattern = bb.read_string();
    r.remaining_attempts = bb.read_u8();
    return r;
}

std::vector<uint8_t> C2S_GuessWord::to_bytes() const {
    ByteBuffer bb;
    bb.write_string(session_token);
    uint32_t hi = (uint32_t)(match_id >> 32); bb.write_u32(hi);
    uint32_t lo = (uint32_t)(match_id & 0xffffffff); bb.write_u32(lo);
    bb.write_string(word);
    return make_packet(PacketType::C2S_GuessWord, bb.buf);
}
C2S_GuessWord C2S_GuessWord::from_payload(ByteBuffer& bb) {
    C2S_GuessWord p;
    p.session_token = bb.read_string();
    uint32_t hi = bb.read_u32(); uint32_t lo = bb.read_u32();
    p.match_id = ((uint64_t)hi<<32) | lo;
    p.word = bb.read_string();
    return p;
}

std::vector<uint8_t> S2C_GuessWordResult::to_bytes() const {
    ByteBuffer bb;
    bb.write_u8(correct?1:0);
    bb.write_string(message);
    bb.write_u8(remaining_attempts);
    return make_packet(PacketType::S2C_GuessWordResult, bb.buf);
}
S2C_GuessWordResult S2C_GuessWordResult::from_payload(ByteBuffer& bb) {
    S2C_GuessWordResult r;
    r.correct = bb.read_u8() != 0;
    r.message = bb.read_string();
    r.remaining_attempts = bb.read_u8();
    return r;
}

std::vector<uint8_t> C2S_RequestDraw::to_bytes() const {
    ByteBuffer bb; bb.write_string(session_token);
    uint32_t hi = (uint32_t)(match_id >> 32); bb.write_u32(hi); uint32_t lo = (uint32_t)(match_id & 0xffffffff); bb.write_u32(lo);
    return make_packet(PacketType::C2S_RequestDraw, bb.buf);
}
C2S_RequestDraw C2S_RequestDraw::from_payload(ByteBuffer& bb) {
    C2S_RequestDraw p; p.session_token = bb.read_string();
    uint32_t hi = bb.read_u32(); uint32_t lo = bb.read_u32(); p.match_id = ((uint64_t)hi<<32)|lo;
    return p;
}
std::vector<uint8_t> S2C_DrawRequest::to_bytes() const {
    ByteBuffer bb; bb.write_string(from_username); uint32_t hi = (uint32_t)(match_id>>32); bb.write_u32(hi); bb.write_u32((uint32_t)(match_id&0xffffffff));
    return make_packet(PacketType::S2C_DrawRequest, bb.buf);
}
S2C_DrawRequest S2C_DrawRequest::from_payload(ByteBuffer& bb) {
    S2C_DrawRequest p; p.from_username = bb.read_string(); uint32_t hi = bb.read_u32(); uint32_t lo = bb.read_u32(); p.match_id = ((uint64_t)hi<<32)|lo; return p;
}

std::vector<uint8_t> C2S_EndGame::to_bytes() const {
    ByteBuffer bb; bb.write_string(session_token); bb.write_u32((uint32_t)(match_id>>32)); bb.write_u32((uint32_t)(match_id&0xffffffff));
    bb.write_u8(result_code);
    bb.write_string(message);
    return make_packet(PacketType::C2S_EndGame, bb.buf);
}
C2S_EndGame C2S_EndGame::from_payload(ByteBuffer& bb) {
    C2S_EndGame p; p.session_token = bb.read_string(); uint32_t hi = bb.read_u32(); uint32_t lo = bb.read_u32(); p.match_id = ((uint64_t)hi<<32)|lo; p.result_code = bb.read_u8(); p.message = bb.read_string(); return p;
}

std::vector<uint8_t> S2C_GameEnd::to_bytes() const {
    ByteBuffer bb; bb.write_u32((uint32_t)(match_id>>32)); bb.write_u32((uint32_t)(match_id&0xffffffff)); bb.write_u8(result_code); bb.write_string(summary);
    return make_packet(PacketType::S2C_GameEnd, bb.buf);
}
S2C_GameEnd S2C_GameEnd::from_payload(ByteBuffer& bb) {
    S2C_GameEnd p; uint32_t hi = bb.read_u32(); uint32_t lo = bb.read_u32(); p.match_id = ((uint64_t)hi<<32)|lo; p.result_code = bb.read_u8(); p.summary = bb.read_string(); return p;
}

// ---------------- History / Leaderboard ----------------
void S2C_HistoryList::Entry::write(ByteBuffer& bb) const {
    bb.write_u32((uint32_t)(match_id>>32)); bb.write_u32((uint32_t)(match_id&0xffffffff));
    bb.write_string(opponent);
    bb.write_u8(result_code);
    bb.write_u32((uint32_t)timestamp); // truncate to 32 bits for example
    bb.write_string(summary);
}
S2C_HistoryList::Entry S2C_HistoryList::Entry::read(ByteBuffer& bb) {
    Entry e; uint32_t hi = bb.read_u32(); uint32_t lo = bb.read_u32(); e.match_id = ((uint64_t)hi<<32)|lo;
    e.opponent = bb.read_string();
    e.result_code = bb.read_u8();
    e.timestamp = bb.read_u32();
    e.summary = bb.read_string();
    return e;
}
std::vector<uint8_t> S2C_HistoryList::to_bytes() const {
    ByteBuffer bb; bb.write_u16((uint16_t)entries.size());
    for (auto &en: entries) en.write(bb);
    return make_packet(PacketType::S2C_HistoryList, bb.buf);
}
S2C_HistoryList S2C_HistoryList::from_payload(ByteBuffer& bb) {
    S2C_HistoryList r;
    uint16_t n = bb.read_u16();
    for (int i=0;i<n;i++) r.entries.push_back(S2C_HistoryList::Entry::read(bb));
    return r;
}

void S2C_Leaderboard::Row::write(ByteBuffer& bb) const {
    bb.write_string(username);
    bb.write_u32(wins);
    bb.write_u32(losses);
    bb.write_u32(draws);
}
S2C_Leaderboard::Row S2C_Leaderboard::Row::read(ByteBuffer& bb) {
    Row r; r.username = bb.read_string(); r.wins = bb.read_u32(); r.losses = bb.read_u32(); r.draws = bb.read_u32(); return r;
}
std::vector<uint8_t> S2C_Leaderboard::to_bytes() const {
    ByteBuffer bb; bb.write_u16((uint16_t)rows.size());
    for (auto &r: rows) r.write(bb);
    return make_packet(PacketType::S2C_Leaderboard, bb.buf);
}
S2C_Leaderboard S2C_Leaderboard::from_payload(ByteBuffer& bb) {
    S2C_Leaderboard out;
    uint16_t n = bb.read_u16();
    for (int i=0;i<n;i++) out.rows.push_back(S2C_Leaderboard::Row::read(bb));
    return out;
}

// Generic ack / error
std::vector<uint8_t> S2C_Ack::to_bytes() const {
    ByteBuffer bb; bb.write_u16(ack_for_type); bb.write_u8(static_cast<uint8_t>(code)); bb.write_string(message);
    return make_packet(PacketType::S2C_Ack, bb.buf);
}
S2C_Ack S2C_Ack::from_payload(ByteBuffer& bb) {
    S2C_Ack a; a.ack_for_type = bb.read_u16(); a.code = static_cast<ResultCode>(bb.read_u8()); a.message = bb.read_string(); return a;
}

std::vector<uint8_t> S2C_Error::to_bytes() const {
    ByteBuffer bb; bb.write_u16(for_type); bb.write_string(message);
    return make_packet(PacketType::S2C_Error, bb.buf);
}
S2C_Error S2C_Error::from_payload(ByteBuffer& bb) {
    S2C_Error e; e.for_type = bb.read_u16(); e.message = bb.read_string(); return e;
}

} // namespace hangman
