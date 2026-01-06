#pragma once

#include "network/ClientSocket.h"
#include "protocol/packets.h"
#include <memory>
#include <string>
#include <mutex>

namespace hangman {

class GameClient {
public:
    static GameClient& getInstance();

    GameClient(const GameClient&) = delete;
    GameClient& operator=(const GameClient&) = delete;

    // Connection
    bool connect(const std::string& host = "127.0.0.1", int port = 5000);
    void disconnect();
    bool isConnected() const;

    // Authentication
    S2C_RegisterResult registerUser(const std::string& username, const std::string& password);
    S2C_LoginResult login(const std::string& username, const std::string& password);
    S2C_LogoutAck logout();

    // Room Management
    S2C_CreateRoomResult createRoom(const std::string& roomName);
    S2C_LeaveRoomAck leaveRoom(uint32_t roomId);

    // Get current session token
    const std::string& getSessionToken() const { return sessionToken; }
    bool hasValidSession() const { return !sessionToken.empty(); }

private:
    GameClient();
    ~GameClient() = default;

    // Send packet and receive response
    template<typename ResponseType>
    ResponseType sendAndReceive(const std::vector<uint8_t>& packet);

    std::unique_ptr<ClientSocket> socket;
    std::string sessionToken;
    std::mutex socketMutex;
};

} // namespace hangman
