#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace hangman {

class ClientSocket {
public:
    ClientSocket();
    ~ClientSocket();

    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const { return sockfd >= 0; }

    // Send raw data
    bool send(const uint8_t* data, size_t len);
    bool send(const std::vector<uint8_t>& data);

    // Receive data (blocking)
    bool receive(std::vector<uint8_t>& buffer, size_t expectedLen);
    bool receiveExact(uint8_t* buffer, size_t len);
    
    // Check if data is available (non-blocking)
    bool hasData(int timeoutMs = 0);
    
    // Get socket file descriptor
    int getSocketFd() const { return sockfd; }

private:
    int sockfd;
};

} // namespace hangman
