#include "network/Connection.h"
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <cerrno>
#include <arpa/inet.h>

namespace hangman {

Connection::Connection(int clientFd) : clientFd(clientFd) {
    if (clientFd < 0) {
        throw std::invalid_argument("Invalid client file descriptor");
    }
    recvBuffer.reserve(RECV_BUFFER_SIZE);
    sendBuffer.reserve(SEND_BUFFER_SIZE);
}

Connection::~Connection() {
    close();
}

void Connection::close() {
    if (clientFd >= 0) {
        ::close(clientFd);
        clientFd = -1;
    }
}

bool Connection::sendData(const uint8_t* data, size_t len) {
    if (clientFd < 0) {
        throw std::runtime_error("Connection is closed");
    }

    // Try to write directly first
    ssize_t written = ::write(clientFd, data, len);
    
    if (written < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Buffer is full, queue the data
            sendBuffer.insert(sendBuffer.end(), data, data + len);
            return false;
        }
        // Real error
        close();
        throw std::runtime_error("Failed to write to socket");
    }

    if ((size_t)written < len) {
        // Partial write, queue the rest
        sendBuffer.insert(sendBuffer.end(), data + written, data + len);
        return false;
    }

    // All sent successfully
    return true;
}

const uint8_t* Connection::getPendingSendData(size_t& outLen) const {
    if (sendPos >= sendBuffer.size()) {
        outLen = 0;
        return nullptr;
    }
    outLen = sendBuffer.size() - sendPos;
    return sendBuffer.data() + sendPos;
}

void Connection::confirmSent(size_t bytes) {
    sendPos += bytes;
    
    // Clean up sent data periodically
    if (sendPos > SEND_BUFFER_SIZE / 2) {
        if (sendPos >= sendBuffer.size()) {
            sendBuffer.clear();
            sendPos = 0;
        } else {
            sendBuffer.erase(sendBuffer.begin(), sendBuffer.begin() + sendPos);
            sendPos = 0;
        }
    }
}

bool Connection::receiveData() {
    if (clientFd < 0) {
        throw std::runtime_error("Connection is closed");
    }

    // Read from socket
    uint8_t buffer[4096];
    ssize_t nread = ::read(clientFd, buffer, sizeof(buffer));

    if (nread < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available right now
            return true;
        }
        // Real error
        close();
        return false;
    }

    if (nread == 0) {
        // Connection closed by client
        close();
        return false;
    }

    // Append data to receive buffer
    recvBuffer.insert(recvBuffer.end(), buffer, buffer + nread);
    return true;
}

const uint8_t* Connection::getReceivedData(size_t& outLen) const {
    if (recvPos >= recvBuffer.size()) {
        outLen = 0;
        return nullptr;
    }
    outLen = recvBuffer.size() - recvPos;
    return recvBuffer.data() + recvPos;
}

void Connection::confirmProcessed(size_t bytes) {
    recvPos += bytes;
    
    // Clean up processed data periodically
    if (recvPos > RECV_BUFFER_SIZE / 2) {
        if (recvPos >= recvBuffer.size()) {
            recvBuffer.clear();
            recvPos = 0;
        } else {
            recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + recvPos);
            recvPos = 0;
        }
    }
}

bool Connection::hasCompletePacket() const {
    // Need at least header size (1 + 2 + 4 = 7 bytes)
    const size_t HEADER_SIZE = 7;
    
    if (recvBuffer.size() - recvPos < HEADER_SIZE) {
        return false;
    }

    // Parse header to get payload length
    const uint8_t* data = recvBuffer.data() + recvPos;
    uint32_t payloadLen;
    std::memcpy(&payloadLen, data + 3, 4);
    
    // Convert from network byte order
    payloadLen = ntohl(payloadLen);
    
    // Check if we have the complete packet (header + payload)
    return recvBuffer.size() - recvPos >= HEADER_SIZE + payloadLen;
}

} // namespace hangman
