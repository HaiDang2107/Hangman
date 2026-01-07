#include "network/ClientSocket.h"
#include <iostream>
#include <cstring>
#include <sys/select.h>
#include <sys/time.h>

namespace hangman {

ClientSocket::ClientSocket() : sockfd(-1) {}

ClientSocket::~ClientSocket() {
    disconnect();
}

bool ClientSocket::connect(const std::string& host, int port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << host << std::endl;
        close(sockfd);
        sockfd = -1;
        return false;
    }

    if (::connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed to " << host << ":" << port << std::endl;
        close(sockfd);
        sockfd = -1;
        return false;
    }

    std::cout << "Connected to server " << host << ":" << port << std::endl;
    return true;
}

void ClientSocket::disconnect() {
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
}

bool ClientSocket::send(const uint8_t* data, size_t len) {
    if (sockfd < 0) return false;

    size_t totalSent = 0;
    while (totalSent < len) {
        ssize_t sent = ::send(sockfd, data + totalSent, len - totalSent, 0);
        if (sent <= 0) {
            std::cerr << "Send failed" << std::endl;
            return false;
        }
        totalSent += sent;
    }
    return true;
}

bool ClientSocket::send(const std::vector<uint8_t>& data) {
    return send(data.data(), data.size());
}

bool ClientSocket::receive(std::vector<uint8_t>& buffer, size_t expectedLen) {
    if (sockfd < 0) return false;

    buffer.resize(expectedLen);
    return receiveExact(buffer.data(), expectedLen);
}

bool ClientSocket::receiveExact(uint8_t* buffer, size_t len) {
    if (sockfd < 0) return false;

    size_t totalReceived = 0;
    while (totalReceived < len) {
        ssize_t received = recv(sockfd, buffer + totalReceived, len - totalReceived, 0);
        if (received <= 0) {
            if (received == 0) {
                std::cerr << "Connection closed by server" << std::endl;
            } else {
                std::cerr << "Receive failed" << std::endl;
            }
            return false;
        }
        totalReceived += received;
    }
    return true;
}

bool ClientSocket::hasData(int timeoutMs) {
    if (sockfd < 0) return false;
    
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    
    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    int result = select(sockfd + 1, &readfds, nullptr, nullptr, &timeout);
    return result > 0;
}

} // namespace hangman
