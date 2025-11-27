#pragma once

#include <vector>
#include <cstdint>
#include <memory>

namespace hangman {

class Connection {
public:
    explicit Connection(int clientFd);
    ~Connection();

    // Socket management
    int getFd() const { return clientFd; }
    void close();
    bool isClosed() const { return clientFd < 0; }

    // Sending data - returns true if all sent, false if partial/pending
    bool sendData(const uint8_t* data, size_t len);
    
    // Get pending data to send (returns pointer and size)
    const uint8_t* getPendingSendData(size_t& outLen) const;
    
    // Confirm bytes sent
    void confirmSent(size_t bytes);
    
    // Receiving data - reads from socket into buffer
    bool receiveData();
    
    // Get received data available for processing
    const uint8_t* getReceivedData(size_t& outLen) const;
    
    // Mark bytes as processed
    void confirmProcessed(size_t bytes);
    
    // Check if we have a complete packet (need at least header size)
    bool hasCompletePacket() const;

    // Buffer sizes
    static constexpr size_t RECV_BUFFER_SIZE = 8192;
    static constexpr size_t SEND_BUFFER_SIZE = 8192;

private:
    int clientFd;
    std::vector<uint8_t> recvBuffer;
    size_t recvPos = 0;  // Position of unprocessed data
    
    std::vector<uint8_t> sendBuffer;
    size_t sendPos = 0;  // Position of unsent data
};

using ConnectionPtr = std::shared_ptr<Connection>;

} // namespace hangman
