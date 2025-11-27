#include <iostream>
#include <vector>
#include "packets.h"

using namespace hangman;

int main() {
    try {
        // 1) Client creates login packet
        C2S_Login login;
        login.username = "alice";
        login.password = "password123"; // in real world hash first

        auto bytes = login.to_bytes();
        std::cout << "Login packet bytes size: " << bytes.size() << std::endl;

        // 2) Server reads header
        PacketHeader hdr = PacketHeader::parse_header(bytes.data(), bytes.size());
        std::cout << "Parsed hdr version=" << int(hdr.version) << " type=" << uint16_t(hdr.type) << " payload_len=" << hdr.payload_len << std::endl;

        // 3) parse payload into ByteBuffer
        ByteBuffer bb;
        // payload starts after header
        size_t payload_offset = PacketHeader::HEADER_SIZE;
        bb.buf.insert(bb.buf.end(), bytes.begin() + payload_offset, bytes.end());
        C2S_Login parsed = C2S_Login::from_payload(bb);
        std::cout << "Parsed username=" << parsed.username << " password=" << parsed.password << std::endl;

        // 4) Server replies with LoginResult
        S2C_LoginResult reply;
        reply.code = ResultCode::OK;
        reply.message = "Welcome";
        reply.session_token = "sess-token-abc-123";
        auto reply_bytes = reply.to_bytes();
        std::cout << "Reply bytes size: " << reply_bytes.size() << std::endl;

        // 5) Parse reply header/payload
        PacketHeader rh = PacketHeader::parse_header(reply_bytes.data(), reply_bytes.size());
        ByteBuffer rbb;
        rbb.buf.insert(rbb.buf.end(), reply_bytes.begin()+PacketHeader::HEADER_SIZE, reply_bytes.end());
        S2C_LoginResult rr = S2C_LoginResult::from_payload(rbb);
        std::cout << "Reply parsed code=" << int(rr.code) << " token=" << rr.session_token << std::endl;

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
