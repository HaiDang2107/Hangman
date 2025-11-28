#include "network/Server.h"
#include "network/Socket.h"
#include "threading/Task.h"
#include "threading/CallbackQueue.h"
#include "service/AuthService.h"
#include "protocol/packets.h"
#include "protocol/bytebuffer.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

namespace hangman
{
    // CONSTRUCTOR: CREATE LISTENING SOCKET
    Server::Server(int port)
        : port(port), listenFd(-1), running(false),
          eventLoop(std::make_unique<EventLoop>()),
          taskQueue(std::make_unique<TaskQueue>()),
          callbackQueue(std::make_unique<CallbackQueue>())
    {

        // Create listening socket
        listenFd = Socket::createListeningSocket(port);
        std::cout << "Server listening on port " << port << std::endl;

        // Register listen fd with event loop
        eventLoop->addFd(listenFd, [this]()
                         { handleAccept(); });

        // Register callback queue notification fd (WAKE UP main thread when worker thread completed tasks)
        eventLoop->addFd(callbackQueue->getNotificationFd(), [this]()
                         { handleCallbacks(); });
    }

    Server::~Server()
    {
        stop();
        if (listenFd >= 0)
        {
            Socket::closeSocket(listenFd);
        }
    }

    bool Server::initialize(const std::string& dbPath)
    {
        // Load database
        if (!AuthService::getInstance().loadDatabase(dbPath)) {
            std::cerr << "Failed to load database from: " << dbPath << std::endl;
            return false;
        }

        std::cout << "Database loaded successfully from: " << dbPath << std::endl;
        initialized = true;
        return true;
    }

    void Server::run()
    {
        running = true;

        // Start worker thread
        workerThread = std::thread([this]()
                                   { workerThreadLoop(); });

        // Run event loop (Main thread is blocked here)
        eventLoop->run();

        // Stop worker thread
        taskQueue->stop();
        if (workerThread.joinable())
        {
            workerThread.join();
        }

        std::cout << "Server stopped" << std::endl;
    }

    void Server::stop()
    {
        running = false;
        eventLoop->stop();
    }

    void Server::handleAccept()
    {   // DRAIN THE QUEUE
        while (true)
        {
            int clientFd = Socket::acceptConnection(listenFd); // RETURN NEW CLIENT FD IF THERE IS A PENDING CONNECTION
            if (clientFd < 0)
            {
                break; // No more pending connections
            }

            std::cout << "New client connection: fd=" << clientFd << std::endl;

            try
            {
                // Create connection wrapper
                auto conn = std::make_shared<Connection>(clientFd);
                connections[clientFd] = conn; // STORE THE CONNECTION

                // Register with event loop
                eventLoop->addFd(clientFd, [this, clientFd]()
                                 { handleClientRead(clientFd); });
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to accept connection: " << e.what() << std::endl;
                Socket::closeSocket(clientFd);
            }
        }
    }

    // HANDLE: This socket is ready to read
    void Server::handleClientRead(int clientFd)
    {
        auto it = connections.find(clientFd);
        if (it == connections.end())
        {
            return;
        }

        auto &conn = it->second;

        try
        {
            // Read from socket
            if (!conn->receiveData())
            {
                // Connection closed
                std::cout << "Client disconnected: fd=" << clientFd << std::endl;
                eventLoop->removeFd(clientFd);
                connections.erase(it);
                return;
            }

            // Process complete packets
            while (conn->hasCompletePacket())
            {
                size_t dataLen;
                const uint8_t *data = conn->getReceivedData(dataLen);

                if (data == nullptr || dataLen == 0)
                {
                    break;
                }

                // Extract header
                ByteBuffer headerBuf; // NULL
                headerBuf.buf.insert(headerBuf.buf.begin(), data, data + 7);

                uint8_t version = headerBuf.read_u8();
                uint16_t packetType = headerBuf.read_u16(); // Read packetType
                uint32_t payloadLen = headerBuf.read_u32();

                // Verify header
                if (version != PROTOCOL_VERSION)
                {
                    std::cerr << "Invalid protocol version" << std::endl;
                    conn->confirmProcessed(7);
                    continue;
                }

                // Process packet (7 = header size)
                processPacket(clientFd, packetType, data + 7, payloadLen);

                // Mark packet as processed
                conn->confirmProcessed(7 + payloadLen);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling client read: " << e.what() << std::endl;
            eventLoop->removeFd(clientFd);
            connections.erase(it);
        }
    }

    // HANDLE: This socket is ready to write (socket buffer has space)
    void Server::handleClientWrite(int clientFd)
    {
        auto it = connections.find(clientFd);
        if (it == connections.end())
        {
            return;
        }

        auto &conn = it->second;

        try
        {
            size_t pendingLen;
            const uint8_t *pending = conn->getPendingSendData(pendingLen);

            if (pending != nullptr && pendingLen > 0)
            {
                ssize_t sent = write(clientFd, pending, pendingLen);
                if (sent > 0)
                {
                    conn->confirmSent(sent);
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling client write: " << e.what() << std::endl;
        }
    }

    void Server::handleCallbacks()
    {
        callbackQueue->resetNotification();

        auto callbacks = callbackQueue->popAll();
        for (auto &callback : callbacks)
        {
            try
            {
                callback->execute();
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error executing callback: " << e.what() << std::endl;
            }
        }
    }

    void Server::processPacket(int clientFd, uint16_t packetType, const uint8_t *data, size_t len)
    {
        if (len == 0)
        {
            return;
        }

        try
        {
            ByteBuffer buf;
            buf.buf.insert(buf.buf.end(), data, data + len);

            switch (packetType) {
                case static_cast<uint16_t>(PacketType::C2S_Register): {
                    C2S_Register registerReq = C2S_Register::from_payload(buf);
                    auto task = std::make_shared<RegisterTask>(clientFd, registerReq);
                    taskQueue->push(task);
                    std::cout << "Queued RegisterTask for client " << clientFd << std::endl;
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_Login): {
                    C2S_Login loginReq = C2S_Login::from_payload(buf);
                    auto task = std::make_shared<LoginTask>(clientFd, loginReq);
                    taskQueue->push(task);
                    std::cout << "Queued LoginTask for client " << clientFd << std::endl;
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_Logout): {
                    C2S_Logout logoutReq = C2S_Logout::from_payload(buf);
                    auto task = std::make_shared<LogoutTask>(clientFd, logoutReq);
                    taskQueue->push(task);
                    std::cout << "Queued LogoutTask for client " << clientFd << std::endl;
                    break;
                }

                default:
                    std::cerr << "Unknown packet type: 0x" << std::hex << packetType << std::dec << std::endl;
                    break;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error processing packet: " << e.what() << std::endl;
        }
    }

    void Server::sendResponse(int clientFd, const std::vector<uint8_t> &packet)
    {
        auto it = connections.find(clientFd);
        if (it == connections.end())
        {
            return;
        }

        try
        {
            it->second->sendData(packet.data(), packet.size());

            // Re-register for write events if there's pending data
            size_t pendingLen;
            if (it->second->getPendingSendData(pendingLen) != nullptr && pendingLen > 0)
            {
                // Update event to include EPOLLOUT
                eventLoop->addFd(clientFd, [this, clientFd]()
                                 { handleClientWrite(clientFd); });
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error sending response: " << e.what() << std::endl;
        }
    }

    void Server::workerThreadLoop()
    {
        std::cout << "Worker thread started" << std::endl;

        while (true)
        {
            TaskPtr task = taskQueue->pop();
            if (!task)
            {
                break; // Queue stopped
            }

            try
            {
                task->execute();

                // Create callback to send response back to client
                auto callback = std::make_shared<FunctionCallback>(
                    [task, this]()
                    {
                        task->onComplete();
                        
                        int clientFd = task->getClientFd();
                        
                        // Send response based on task type
                        if (auto registerTask = dynamic_cast<RegisterTask*>(task.get())) {
                            auto response = registerTask->getResult();
                            std::vector<uint8_t> packet = response.to_bytes();
                            sendResponse(clientFd, packet);
                            std::cout << "Sent RegisterResult to client " << clientFd << std::endl;
                        }
                        else if (auto loginTask = dynamic_cast<LoginTask*>(task.get())) {
                            auto response = loginTask->getResult();
                            std::vector<uint8_t> packet = response.to_bytes();
                            sendResponse(clientFd, packet);
                            std::cout << "Sent LoginResult to client " << clientFd << std::endl;
                        }
                        else if (auto logoutTask = dynamic_cast<LogoutTask*>(task.get())) {
                            auto response = logoutTask->getResult();
                            std::vector<uint8_t> packet = response.to_bytes();
                            sendResponse(clientFd, packet);
                            std::cout << "Sent LogoutAck to client " << clientFd << std::endl;
                        }
                    });

                // Push callback to network thread
                callbackQueue->push(callback);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error executing task: " << e.what() << std::endl;
            }
        }

        std::cout << "Worker thread stopped" << std::endl;
    }

} // namespace hangman
