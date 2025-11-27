// include/network/Socket.h
#pragma once
#include <string>

class Socket {
public:
    static int createListeningSocket(int port);
    static void setNonBlocking(int fd);
    static void setReuseAddr(int fd);
    static int acceptConnection(int listenFd);
    static void closeSocket(int fd);
};