#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

// Include the actual protocol definitions
#include "protocol/packets.h"
#include "protocol/packet_types.h"
#include "protocol/bytebuffer.h"

using namespace hangman;

// Helper function to print result code as string
std::string resultCodeToString(ResultCode code) {
    switch (code) {
        case ResultCode::OK: return "OK";
        case ResultCode::FAIL: return "FAIL";
        case ResultCode::AUTH_FAIL: return "AUTH_FAIL";
        case ResultCode::INVALID: return "INVALID";
        case ResultCode::NOT_FOUND: return "NOT_FOUND";
        case ResultCode::ALREADY: return "ALREADY";
        case ResultCode::SERVER_ERROR: return "SERVER_ERROR";
        default: return "UNKNOWN";
    }
}

// Helper function to print packet type as string
std::string packetTypeToString(PacketType type) {
    switch (type) {
        case PacketType::C2S_Register: return "C2S_Register";
        case PacketType::S2C_RegisterResult: return "S2C_RegisterResult";
        case PacketType::C2S_Login: return "C2S_Login";
        case PacketType::S2C_LoginResult: return "S2C_LoginResult";
        case PacketType::C2S_Logout: return "C2S_Logout";
        case PacketType::S2C_LogoutAck: return "S2C_LogoutAck";
        case PacketType::C2S_CreateRoom: return "C2S_CreateRoom";
        case PacketType::S2C_CreateRoomResult: return "S2C_CreateRoomResult";
        case PacketType::C2S_LeaveRoom: return "C2S_LeaveRoom";
        case PacketType::S2C_LeaveRoomAck: return "S2C_LeaveRoomAck";
        default: return "UNKNOWN";
    }
}

// Send packet and receive response
bool sendAndReceive(int sock, const std::vector<uint8_t>& packet, std::vector<uint8_t>& response) {
    // Send
    if (send(sock, packet.data(), packet.size(), 0) < 0) {
        std::cerr << "Failed to send packet" << std::endl;
        return false;
    }

    // Wait a bit for response
    usleep(100000);  // 100ms

    // Receive
    response.resize(4096);
    int n = recv(sock, response.data(), response.size(), 0);
    if (n <= 0) {
        std::cerr << "Failed to receive response" << std::endl;
        return false;
    }
    response.resize(n);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    std::cout << "=== Hangman Room Test Client ===" << std::endl;
    std::cout << "Connecting to localhost:" << port << std::endl;

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Connect
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    std::cout << "✓ Connected to server" << std::endl << std::endl;

    std::string sessionToken;
    uint32_t roomId = 0;

    // Test 1: Login
    std::cout << "Test 1: Login (testuser2:testpass)" << std::endl;
    {
        C2S_Login req;
        req.username = "testuser2";
        req.password = "testpass";
        
        std::vector<uint8_t> packet = req.to_bytes();
        std::vector<uint8_t> response;
        
        if (sendAndReceive(sock, packet, response)) {
            ByteBuffer buf;
            buf.buf = response;
            
            // Skip header
            buf.read_u8(); // version
            uint16_t type = buf.read_u16();
            buf.read_u32(); // length
            
            if (type == static_cast<uint16_t>(PacketType::S2C_LoginResult)) {
                S2C_LoginResult res = S2C_LoginResult::from_payload(buf);
                std::cout << "  Result: " << resultCodeToString(res.code) << std::endl;
                std::cout << "  Message: " << res.message << std::endl;
                
                if (res.code == ResultCode::OK) {
                    sessionToken = res.session_token;
                    std::cout << "  Session Token: " << sessionToken << std::endl;
                    std::cout << "✓ Login successful" << std::endl;
                } else {
                    std::cout << "✗ Login failed" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "✗ Unexpected packet type: " << packetTypeToString(static_cast<PacketType>(type)) << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Test 2: Create Room
    std::cout << "Test 2: Create Room 'TestRoom'" << std::endl;
    {
        C2S_CreateRoom req;
        req.session_token = sessionToken;
        req.room_name = "TestRoom";
        
        std::vector<uint8_t> packet = req.to_bytes();
        std::vector<uint8_t> response;
        
        if (sendAndReceive(sock, packet, response)) {
            ByteBuffer buf;
            buf.buf = response;
            
            // Skip header
            buf.read_u8(); // version
            uint16_t type = buf.read_u16();
            buf.read_u32(); // length
            
            if (type == static_cast<uint16_t>(PacketType::S2C_CreateRoomResult)) {
                S2C_CreateRoomResult res = S2C_CreateRoomResult::from_payload(buf);
                std::cout << "  Result: " << resultCodeToString(res.code) << std::endl;
                std::cout << "  Message: " << res.message << std::endl;
                
                if (res.code == ResultCode::OK) {
                    roomId = res.room_id;
                    std::cout << "  Room ID: " << roomId << std::endl;
                    std::cout << "✓ Room created successfully" << std::endl;
                } else {
                    std::cout << "✗ Room creation failed" << std::endl;
                }
            } else {
                std::cout << "✗ Unexpected packet type: " << packetTypeToString(static_cast<PacketType>(type)) << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Test 3: Leave Room
    std::cout << "Test 3: Leave Room " << roomId << std::endl;
    {
        C2S_LeaveRoom req;
        req.session_token = sessionToken;
        req.room_id = roomId;
        
        std::vector<uint8_t> packet = req.to_bytes();
        std::vector<uint8_t> response;
        
        if (sendAndReceive(sock, packet, response)) {
            ByteBuffer buf;
            buf.buf = response;
            
            // Skip header
            buf.read_u8(); // version
            uint16_t type = buf.read_u16();
            buf.read_u32(); // length
            
            if (type == static_cast<uint16_t>(PacketType::S2C_LeaveRoomAck)) {
                S2C_LeaveRoomAck res = S2C_LeaveRoomAck::from_payload(buf);
                std::cout << "  Result: " << resultCodeToString(res.code) << std::endl;
                std::cout << "  Message: " << res.message << std::endl;
                
                if (res.code == ResultCode::OK) {
                    std::cout << "✓ Left room successfully" << std::endl;
                } else {
                    std::cout << "✗ Failed to leave room" << std::endl;
                }
            } else {
                std::cout << "✗ Unexpected packet type: " << packetTypeToString(static_cast<PacketType>(type)) << std::endl;
            }
        }
    }
    std::cout << std::endl;

    close(sock);
    return 0;
}
