// src/network/Socket.cpp
#include "network/Socket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

int Socket::createListeningSocket(int port) {
    // Create socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // Set reuse address
    setReuseAddr(fd);
    
    // Bind
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        throw std::runtime_error("Failed to bind");
    }
    
    // Listen
    if (listen(fd, SOMAXCONN) < 0) {
        close(fd);
        throw std::runtime_error("Failed to listen");
    }
    
    // Set non-blocking
    setNonBlocking(fd);
    
    return fd;
}

void Socket::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        throw std::runtime_error("Failed to get socket flags");
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error("Failed to set non-blocking");
    }
}

void Socket::setReuseAddr(int fd) {
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

int Socket::acceptConnection(int listenFd) {
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    int clientFd = accept(listenFd, (sockaddr*)&clientAddr, &addrLen);
    if (clientFd >= 0) {
        setNonBlocking(clientFd);
    }
    
    return clientFd;
}

void Socket::closeSocket(int fd) {
    close(fd);
}