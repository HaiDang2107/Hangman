#include "protocol/packets.h"
#include <stdexcept>

namespace hangman
{

    // =====================================================
    //                      PacketHeader
    // =====================================================
    PacketHeader PacketHeader::parse_header(const uint8_t *data, size_t len)
    {
        if (len < HEADER_SIZE)
        {
            throw std::runtime_error("Insufficient data for packet header");
        }

        ByteBuffer bb;
        bb.buf = std::vector<uint8_t>(data, data + len);

        PacketHeader header;
        header.version = bb.read_u8();
        header.type = static_cast<PacketType>(bb.read_u16());
        header.payload_len = bb.read_u32();
        return header;
    }

    // =====================================================
    //                      C2S_Login
    // =====================================================
    std::vector<uint8_t> C2S_Login::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_string(username);
        bb.write_string(password);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::C2S_Login, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    C2S_Login C2S_Login::from_payload(ByteBuffer &bb)
    {
        C2S_Login packet;
        packet.username = bb.read_string();
        packet.password = bb.read_string();
        return packet;
    }

    // =====================================================
    //                    S2C_LoginResult
    // =====================================================
    std::vector<uint8_t> S2C_LoginResult::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_u8(static_cast<uint8_t>(code));
        bb.write_string(message);
        bb.write_string(session_token);
        bb.write_u16(num_of_wins);
        bb.write_u16(total_points);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::S2C_LoginResult, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    S2C_LoginResult S2C_LoginResult::from_payload(ByteBuffer &bb)
    {
        S2C_LoginResult packet;
        packet.code = static_cast<ResultCode>(bb.read_u8());
        packet.message = bb.read_string();
        packet.session_token = bb.read_string();
        packet.num_of_wins = bb.read_u16();
        packet.total_points = bb.read_u16();
        return packet;
    }

    // =====================================================
    //                    C2S_Register
    // =====================================================
    std::vector<uint8_t> C2S_Register::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_string(username);
        bb.write_string(password);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::C2S_Register, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    C2S_Register C2S_Register::from_payload(ByteBuffer &bb)
    {
        C2S_Register packet;
        packet.username = bb.read_string();
        packet.password = bb.read_string();
        return packet;
    }

    // =====================================================
    //                S2C_RegisterResult
    // =====================================================
    std::vector<uint8_t> S2C_RegisterResult::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_u8(static_cast<uint8_t>(code));
        bb.write_string(message);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::S2C_RegisterResult, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    S2C_RegisterResult S2C_RegisterResult::from_payload(ByteBuffer &bb)
    {
        S2C_RegisterResult packet;
        packet.code = static_cast<ResultCode>(bb.read_u8());
        packet.message = bb.read_string();
        return packet;
    }

    // =====================================================
    //                    C2S_Logout
    // =====================================================
    std::vector<uint8_t> C2S_Logout::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_string(session_token);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::C2S_Logout, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    C2S_Logout C2S_Logout::from_payload(ByteBuffer &bb)
    {
        C2S_Logout packet;
        packet.session_token = bb.read_string();
        return packet;
    }

    // =====================================================
    //                   S2C_LogoutAck
    // =====================================================
    std::vector<uint8_t> S2C_LogoutAck::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_u8(static_cast<uint8_t>(code));
        bb.write_string(message);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::S2C_LogoutAck, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    S2C_LogoutAck S2C_LogoutAck::from_payload(ByteBuffer &bb)
    {
        S2C_LogoutAck packet;
        packet.code = static_cast<ResultCode>(bb.read_u8());
        packet.message = bb.read_string();
        return packet;
    }

    // =====================================================
    //                    C2S_CreateRoom
    // =====================================================
    std::vector<uint8_t> C2S_CreateRoom::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_string(session_token);
        bb.write_string(room_name);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::C2S_CreateRoom, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    C2S_CreateRoom C2S_CreateRoom::from_payload(ByteBuffer &bb)
    {
        C2S_CreateRoom packet;
        packet.session_token = bb.read_string();
        packet.room_name = bb.read_string();
        return packet;
    }

    // =====================================================
    //                S2C_CreateRoomResult
    // =====================================================
    std::vector<uint8_t> S2C_CreateRoomResult::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_u8(static_cast<uint8_t>(code));
        bb.write_string(message);
        bb.write_u32(room_id);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::S2C_CreateRoomResult, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    S2C_CreateRoomResult S2C_CreateRoomResult::from_payload(ByteBuffer &bb)
    {
        S2C_CreateRoomResult packet;
        packet.code = static_cast<ResultCode>(bb.read_u8());
        packet.message = bb.read_string();
        packet.room_id = bb.read_u32();
        return packet;
    }

    // =====================================================
    //                    C2S_LeaveRoom
    // =====================================================
    std::vector<uint8_t> C2S_LeaveRoom::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_string(session_token);
        bb.write_u32(room_id);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::C2S_LeaveRoom, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    C2S_LeaveRoom C2S_LeaveRoom::from_payload(ByteBuffer &bb)
    {
        C2S_LeaveRoom packet;
        packet.session_token = bb.read_string();
        packet.room_id = bb.read_u32();
        return packet;
    }

    // =====================================================
    //                   S2C_LeaveRoomAck
    // =====================================================
    std::vector<uint8_t> S2C_LeaveRoomAck::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_u8(static_cast<uint8_t>(code));
        bb.write_string(message);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::S2C_LeaveRoomAck, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    S2C_LeaveRoomAck S2C_LeaveRoomAck::from_payload(ByteBuffer &bb)
    {
        S2C_LeaveRoomAck packet;
        packet.code = static_cast<ResultCode>(bb.read_u8());
        packet.message = bb.read_string();
        return packet;
    }

    // =====================================================
    //             S2C_PlayerLeftNotification
    // =====================================================
    std::vector<uint8_t> S2C_PlayerLeftNotification::to_bytes() const
    {
        ByteBuffer bb;
        bb.write_string(username);
        bb.write_u8(is_new_host ? 1 : 0);
        bb.write_string(message);

        std::vector<uint8_t> header_bytes =
            PacketHeader::encode_header(PROTOCOL_VERSION, PacketType::S2C_PlayerLeftNotification, bb.size());

        header_bytes.insert(header_bytes.end(), bb.buf.begin(), bb.buf.end());
        return header_bytes;
    }

    S2C_PlayerLeftNotification S2C_PlayerLeftNotification::from_payload(ByteBuffer &bb)
    {
        S2C_PlayerLeftNotification packet;
        packet.username = bb.read_string();
        packet.is_new_host = (bb.read_u8() != 0);
        packet.message = bb.read_string();
        return packet;
    }

    // =====================================================
    //         ALL REMAINING PACKETS (Not Implemented)
    // =====================================================

    std::vector<uint8_t> C2S_RequestOnlineList::to_bytes() const { throw std::runtime_error("Not implemented"); }
    C2S_RequestOnlineList C2S_RequestOnlineList::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_OnlineList::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_OnlineList S2C_OnlineList::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> C2S_SendInvite::to_bytes() const { throw std::runtime_error("Not implemented"); }
    C2S_SendInvite C2S_SendInvite::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_InviteReceived::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_InviteReceived S2C_InviteReceived::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> C2S_RespondInvite::to_bytes() const { throw std::runtime_error("Not implemented"); }
    C2S_RespondInvite C2S_RespondInvite::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_InviteResponse::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_InviteResponse S2C_InviteResponse::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> C2S_SetReady::to_bytes() const { throw std::runtime_error("Not implemented"); }
    C2S_SetReady C2S_SetReady::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_PlayerReadyUpdate::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_PlayerReadyUpdate S2C_PlayerReadyUpdate::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_GameStart::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_GameStart S2C_GameStart::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> C2S_GuessChar::to_bytes() const { throw std::runtime_error("Not implemented"); }
    C2S_GuessChar C2S_GuessChar::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_GuessCharResult::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_GuessCharResult S2C_GuessCharResult::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> C2S_GuessWord::to_bytes() const { throw std::runtime_error("Not implemented"); }
    C2S_GuessWord C2S_GuessWord::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_GuessWordResult::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_GuessWordResult S2C_GuessWordResult::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> C2S_RequestDraw::to_bytes() const { throw std::runtime_error("Not implemented"); }
    C2S_RequestDraw C2S_RequestDraw::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_DrawRequest::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_DrawRequest S2C_DrawRequest::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> C2S_EndGame::to_bytes() const { throw std::runtime_error("Not implemented"); }
    C2S_EndGame C2S_EndGame::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_GameEnd::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_GameEnd S2C_GameEnd::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> C2S_RequestHistory::to_bytes() const { throw std::runtime_error("Not implemented"); }
    C2S_RequestHistory C2S_RequestHistory::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_HistoryList::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_HistoryList S2C_HistoryList::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }
    void S2C_HistoryList::Entry::write(ByteBuffer &) const { throw std::runtime_error("Not implemented"); }
    S2C_HistoryList::Entry S2C_HistoryList::Entry::read(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> C2S_RequestLeaderboard::to_bytes() const { throw std::runtime_error("Not implemented"); }
    C2S_RequestLeaderboard C2S_RequestLeaderboard::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_Leaderboard::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_Leaderboard S2C_Leaderboard::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }
    void S2C_Leaderboard::Row::write(ByteBuffer &) const { throw std::runtime_error("Not implemented"); }
    S2C_Leaderboard::Row S2C_Leaderboard::Row::read(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_Ack::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_Ack S2C_Ack::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

    std::vector<uint8_t> S2C_Error::to_bytes() const { throw std::runtime_error("Not implemented"); }
    S2C_Error S2C_Error::from_payload(ByteBuffer &) { throw std::runtime_error("Not implemented"); }

} // namespace hangman
