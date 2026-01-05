#include "network/GameClient.h"
#include "protocol/bytebuffer.h"
#include <iostream>

namespace hangman {

static GameClient* g_gameClient = nullptr;

GameClient& GameClient::getInstance() {
    if (!g_gameClient) {
        g_gameClient = new GameClient();
    }
    return *g_gameClient;
}

GameClient::GameClient() : socket(std::make_unique<ClientSocket>()) {}

bool GameClient::connect(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(socketMutex);
    return socket->connect(host, port);
}

void GameClient::disconnect() {
    std::lock_guard<std::mutex> lock(socketMutex);
    sessionToken.clear();
    socket->disconnect();
}

bool GameClient::isConnected() const {
    return socket->isConnected();
}

template<typename ResponseType>
ResponseType GameClient::sendAndReceive(const std::vector<uint8_t>& packet) {
    std::lock_guard<std::mutex> lock(socketMutex);
    
    ResponseType response;
    
    // Send packet
    if (!socket->send(packet)) {
        std::cerr << "Failed to send packet" << std::endl;
        return response;
    }

    // Receive header (7 bytes)
    std::vector<uint8_t> headerData;
    if (!socket->receive(headerData, 7)) {
        std::cerr << "Failed to receive header" << std::endl;
        return response;
    }

    ByteBuffer headerBuf;
    headerBuf.buf = headerData;
    
    uint8_t version = headerBuf.read_u8();
    uint16_t packetType = headerBuf.read_u16();
    uint32_t payloadLen = headerBuf.read_u32();

    (void)version;
    (void)packetType;

    // Receive payload
    std::vector<uint8_t> payloadData;
    if (payloadLen > 0) {
        if (!socket->receive(payloadData, payloadLen)) {
            std::cerr << "Failed to receive payload" << std::endl;
            return response;
        }
    }

    // Parse response
    ByteBuffer payloadBuf;
    payloadBuf.buf = payloadData;
    
    try {
        response = ResponseType::from_payload(payloadBuf);
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse response: " << e.what() << std::endl;
    }

    return response;
}

S2C_RegisterResult GameClient::registerUser(const std::string& username, const std::string& password) {
    C2S_Register request;
    request.username = username;
    request.password = password;
    
    return sendAndReceive<S2C_RegisterResult>(request.to_bytes());
}

S2C_LoginResult GameClient::login(const std::string& username, const std::string& password) {
    C2S_Login request;
    request.username = username;
    request.password = password;
    
    auto response = sendAndReceive<S2C_LoginResult>(request.to_bytes());
    
    if (response.code == ResultCode::SUCCESS) {
        sessionToken = response.session_token;
    }
    
    return response;
}

S2C_LogoutAck GameClient::logout() {
    C2S_Logout request;
    request.session_token = sessionToken;
    
    auto response = sendAndReceive<S2C_LogoutAck>(request.to_bytes());
    
    if (response.code == ResultCode::SUCCESS) {
        sessionToken.clear();
    }
    
    return response;
}

S2C_CreateRoomResult GameClient::createRoom(const std::string& roomName) {
    C2S_CreateRoom request;
    request.session_token = sessionToken;
    request.room_name = roomName;
    
    return sendAndReceive<S2C_CreateRoomResult>(request.to_bytes());
}

S2C_LeaveRoomAck GameClient::leaveRoom(uint32_t roomId) {
    C2S_LeaveRoom request;
    request.session_token = sessionToken;
    request.room_id = roomId;
    
    return sendAndReceive<S2C_LeaveRoomAck>(request.to_bytes());
}

} // namespace hangman
