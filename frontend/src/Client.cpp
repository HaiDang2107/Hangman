#include "Client.h"
#include "protocol/packets.h"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

using namespace hangman;

bool Client::connectToServer(const std::string& ip, int port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    return connect(sockfd, (sockaddr*)&addr, sizeof(addr)) == 0;
}

void Client::run() {
    while (true) {
        showAuthMenu();
        int choice;
        std::cin >> choice;
        handleAuthChoice(choice);
    }
}

void Client::showAuthMenu() {
    std::cout << "\n=== HANGMAN ===\n";
    std::cout << "1. Register\n";
    std::cout << "2. Login\n";
    std::cout << "0. Exit\n";
    std::cout << "Choose: ";
}

void Client::handleAuthChoice(int choice) {
    switch (choice) {
        case 1: sendRegister(); break;
        case 2: sendLogin(); break;
        case 0: exit(0);
        default: std::cout << "Invalid choice\n";
    }
}

void Client::sendRegister() {
    C2S_Register pkt;
    std::cout << "Username: ";
    std::cin >> pkt.username;
    std::cout << "Password: ";
    std::cin >> pkt.password;

    sendPacket(pkt.to_bytes());
    receiveOnce();
}

void Client::sendLogin() {
    C2S_Login pkt;
    std::cout << "Username: ";
    std::cin >> pkt.username;
    std::cout << "Password: ";
    std::cin >> pkt.password;

    sendPacket(pkt.to_bytes());
    receiveOnce();
}

bool Client::sendPacket(const std::vector<uint8_t>& data) {
    return send(sockfd, data.data(), data.size(), 0) == (ssize_t)data.size();
}

void Client::receiveOnce() {
    uint8_t buf[4096];
    ssize_t n = recv(sockfd, buf, sizeof(buf), 0);
    if (n <= 0) return;
    processPacket(buf, n);
}

void Client::processPacket(const uint8_t* data, size_t len) {
    PacketHeader hdr = PacketHeader::parse_header(data, len);
    ByteBuffer bb;
    bb.buf.assign(data + PacketHeader::HEADER_SIZE,
                  data + PacketHeader::HEADER_SIZE + hdr.payload_len);

    switch (hdr.type) {
        case PacketType::S2C_RegisterResult:
            onRegisterResult(bb);
            break;
        case PacketType::S2C_LoginResult:
            onLoginResult(bb);
            break;
        case PacketType::S2C_LogoutAck:
            onLogoutAck(bb);
            break;
        default:
            std::cout << "Unhandled packet\n";
    }
}

void Client::onRegisterResult(ByteBuffer& bb) {
    auto res = S2C_RegisterResult::from_payload(bb);
    std::cout << res.message << "\n";
}

void Client::onLoginResult(ByteBuffer& bb) {
    auto res = S2C_LoginResult::from_payload(bb);
    std::cout << res.message << "\n";

    if (res.code == ResultCode::OK) {
        sessionToken = res.session_token;
        std::cout << "Wins: " << res.num_of_wins
                  << " | Points: " << res.total_points << "\n";
    }
}

void Client::sendLogout() {
    C2S_Logout pkt;
    pkt.session_token = sessionToken;
    sendPacket(pkt.to_bytes());
    receiveOnce();
}

void Client::onLogoutAck(ByteBuffer& bb) {
    auto res = S2C_LogoutAck::from_payload(bb);
    std::cout << res.message << "\n";
}
