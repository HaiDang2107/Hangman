#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "protocol/packet_types.h"
#include "protocol/bytebuffer.h"

namespace hangman {

class Client {
public:
    bool connectToServer(const std::string& ip, int port);
    void run();

private:
    int sockfd = -1;
    std::vector<uint8_t> recvBuffer;
    std::string sessionToken;

    // UI
    void showAuthMenu();
    void handleAuthChoice(int choice);

    // Auth actions
    void sendRegister();
    void sendLogin();
    void sendLogout();

    // Network
    bool sendPacket(const std::vector<uint8_t>& data);
    void receiveOnce();
    void processPacket(const uint8_t* data, size_t len);

    // Packet handlers
    void onRegisterResult(ByteBuffer& bb);
    void onLoginResult(ByteBuffer& bb);
    void onLogoutAck(ByteBuffer& bb);
};

} // namespace hangman
