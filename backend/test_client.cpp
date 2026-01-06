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
        case ResultCode::SUCCESS: return "OK";
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

    std::cout << "=== Hangman Network Test Client ===" << std::endl;
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

    // Test 1: Register new user
    std::cout << "Test 1: Register new user (testuser2:testpass)" << std::endl;
    {
        C2S_Register req;
        req.username = "testuser2";
        req.password = "testpass";
        auto packet = req.to_bytes();

        std::vector<uint8_t> response;
        if (sendAndReceive(sock, packet, response)) {
            try {
                PacketHeader header = PacketHeader::parse_header(response.data(), response.size());
                if (header.type == PacketType::S2C_RegisterResult) {
                    ByteBuffer bb;
                    bb.buf = std::vector<uint8_t>(response.begin() + PacketHeader::HEADER_SIZE, response.end());
                    S2C_RegisterResult result = S2C_RegisterResult::from_payload(bb);
                    std::cout << "  Result: code=" << resultCodeToString(result.code) 
                              << " message=" << result.message << std::endl;
                    if (result.code == ResultCode::SUCCESS) std::cout << "  ✓ PASS" << std::endl;
                    else std::cout << "  ✗ FAIL (expected OK)" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "  ✗ Exception: " << e.what() << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Test 2: Register duplicate user
    std::cout << "Test 2: Register duplicate user (testuser:testpass)" << std::endl;
    {
        C2S_Register req;
        req.username = "testuser";
        req.password = "testpass";
        auto packet = req.to_bytes();

        std::vector<uint8_t> response;
        if (sendAndReceive(sock, packet, response)) {
            try {
                PacketHeader header = PacketHeader::parse_header(response.data(), response.size());
                if (header.type == PacketType::S2C_RegisterResult) {
                    ByteBuffer bb;
                    bb.buf = std::vector<uint8_t>(response.begin() + PacketHeader::HEADER_SIZE, response.end());
                    S2C_RegisterResult result = S2C_RegisterResult::from_payload(bb);
                    std::cout << "  Result: code=" << resultCodeToString(result.code) << std::endl;
                    if (result.code == ResultCode::ALREADY) std::cout << "  ✓ PASS" << std::endl;
                    else std::cout << "  ✗ FAIL (expected ALREADY)" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "  ✗ Exception: " << e.what() << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Test 3: Login with correct credentials
    std::cout << "Test 3: Login with correct credentials (user2:123)" << std::endl;
    std::string sessionToken;
    {
        C2S_Login req;
        req.username = "user2";
        req.password = "123";
        auto packet = req.to_bytes();

        std::vector<uint8_t> response;
        if (sendAndReceive(sock, packet, response)) {
            try {
                PacketHeader header = PacketHeader::parse_header(response.data(), response.size());
                if (header.type == PacketType::S2C_LoginResult) {
                    ByteBuffer bb;
                    bb.buf = std::vector<uint8_t>(response.begin() + PacketHeader::HEADER_SIZE, response.end());
                    S2C_LoginResult result = S2C_LoginResult::from_payload(bb);
                    sessionToken = result.session_token;
                    std::cout << "  Result: code=" << resultCodeToString(result.code) 
                              << " message=" << result.message << std::endl;
                    std::cout << "  Session token: " << sessionToken << std::endl;
                    std::cout << "  Wins: " << result.num_of_wins << ", Points: " << result.total_points << std::endl;
                    if (result.code == ResultCode::SUCCESS && !sessionToken.empty()) 
                        std::cout << "  ✓ PASS" << std::endl;
                    else 
                        std::cout << "  ✗ FAIL (expected OK and token)" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "  ✗ Exception: " << e.what() << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Test 4: Logout with invalid token
    std::cout << "Test 4: Logout with invalid token" << std::endl;
    {
        C2S_Logout req;
        req.session_token = "invalid_token";
        auto packet = req.to_bytes();

        std::vector<uint8_t> response;
        if (sendAndReceive(sock, packet, response)) {
            try {
                PacketHeader header = PacketHeader::parse_header(response.data(), response.size());
                if (header.type == PacketType::S2C_LogoutAck) {
                    ByteBuffer bb;
                    bb.buf = std::vector<uint8_t>(response.begin() + PacketHeader::HEADER_SIZE, response.end());
                    S2C_LogoutAck result = S2C_LogoutAck::from_payload(bb);
                    std::cout << "  Result: code=" << resultCodeToString(result.code) << std::endl;
                    if (result.code == ResultCode::AUTH_FAIL) std::cout << "  ✓ PASS" << std::endl;
                    else std::cout << "  ✗ FAIL (expected AUTH_FAIL)" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "  ✗ Exception: " << e.what() << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Test 5: Login with wrong password
    std::cout << "Test 5: Login with wrong password (testuser:wrongpass)" << std::endl;
    {
        C2S_Login req;
        req.username = "testuser";
        req.password = "wrongpass";
        auto packet = req.to_bytes();

        std::vector<uint8_t> response;
        if (sendAndReceive(sock, packet, response)) {
            try {
                PacketHeader header = PacketHeader::parse_header(response.data(), response.size());
                if (header.type == PacketType::S2C_LoginResult) {
                    ByteBuffer bb;
                    bb.buf = std::vector<uint8_t>(response.begin() + PacketHeader::HEADER_SIZE, response.end());
                    S2C_LoginResult result = S2C_LoginResult::from_payload(bb);
                    std::cout << "  Result: code=" << resultCodeToString(result.code) << std::endl;
                    if (result.code == ResultCode::AUTH_FAIL) std::cout << "  ✓ PASS" << std::endl;
                    else std::cout << "  ✗ FAIL (expected AUTH_FAIL)" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "  ✗ Exception: " << e.what() << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Test 6: Login with non-existent user
    std::cout << "Test 6: Login with non-existent user (nobody:anypass)" << std::endl;
    {
        C2S_Login req;
        req.username = "nobody";
        req.password = "anypass";
        auto packet = req.to_bytes();

        std::vector<uint8_t> response;
        if (sendAndReceive(sock, packet, response)) {
            try {
                PacketHeader header = PacketHeader::parse_header(response.data(), response.size());
                if (header.type == PacketType::S2C_LoginResult) {
                    ByteBuffer bb;
                    bb.buf = std::vector<uint8_t>(response.begin() + PacketHeader::HEADER_SIZE, response.end());
                    S2C_LoginResult result = S2C_LoginResult::from_payload(bb);
                    std::cout << "  Result: code=" << resultCodeToString(result.code) << std::endl;
                    if (result.code == ResultCode::AUTH_FAIL) std::cout << "  ✓ PASS" << std::endl;
                    else std::cout << "  ✗ FAIL (expected AUTH_FAIL)" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "  ✗ Exception: " << e.what() << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Test 7: Logout with valid token
    if (!sessionToken.empty()) {
        std::cout << "Test 7: Logout with valid token" << std::endl;
        {
            C2S_Logout req;
            req.session_token = sessionToken;
            auto packet = req.to_bytes();

            std::vector<uint8_t> response;
            if (sendAndReceive(sock, packet, response)) {
                try {
                    PacketHeader header = PacketHeader::parse_header(response.data(), response.size());
                    if (header.type == PacketType::S2C_LogoutAck) {
                        ByteBuffer bb;
                        bb.buf = std::vector<uint8_t>(response.begin() + PacketHeader::HEADER_SIZE, response.end());
                        S2C_LogoutAck result = S2C_LogoutAck::from_payload(bb);
                        std::cout << "  Result: code=" << resultCodeToString(result.code) << std::endl;
                        if (result.code == ResultCode::SUCCESS) std::cout << "  ✓ PASS" << std::endl;
                        else std::cout << "  ✗ FAIL (expected OK)" << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "  ✗ Exception: " << e.what() << std::endl;
                }
            }
        }
        std::cout << std::endl;
    }

    std::cout << "=== Tests Complete ===" << std::endl;

    close(sock);
    return 0;
}
