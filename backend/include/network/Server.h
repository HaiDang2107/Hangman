#pragma once

#include "network/EventLoop.h"
#include "network/Connection.h"
#include "threading/TaskQueue.h"
#include "threading/CallbackQueue.h"
#include <map>
#include <memory>
#include <thread>
#include <atomic>

namespace hangman
{

    class Server
    {
    public:
        explicit Server(int port);
        ~Server();

        // Initialize server (load database and prepare)
        bool initialize(const std::string& dbPath = "database/account.txt");

        // Start the server (blocks until stopped)
        void run();

        // Stop the server
        void stop();

        // Get server state
        bool isRunning() const { return running; }
        int getPort() const { return port; }
        bool isInitialized() const { return initialized; }

    private:
        // Network thread handlers
        void handleAccept();
        void handleClientRead(int clientFd);
        void handleClientWrite(int clientFd);
        void handleCallbacks();

        // Worker thread main loop
        void workerThreadLoop();

        // Helper methods
        void processPacket(int clientFd, uint16_t packetType, const uint8_t *data, size_t len);
        void sendResponse(int clientFd, const std::vector<uint8_t> &packet);

        int port;
        int listenFd;
        std::atomic<bool> running;
        bool initialized = false;

        // Event loop for network I/O
        std::unique_ptr<EventLoop> eventLoop;

        // Connection management
        std::map<int, ConnectionPtr> connections;

        // Task and callback queues
        std::unique_ptr<TaskQueue> taskQueue;
        std::unique_ptr<CallbackQueue> callbackQueue;

        // Worker thread
        std::thread workerThread;
    };

} // namespace hangman
