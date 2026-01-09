#include "network/Server.h"
#include "network/Socket.h"
#include "threading/Task.h"
#include "threading/CallbackQueue.h"
#include "service/AuthService.h"
#include "service/RoomService.h"
#include "protocol/packets.h"
#include "protocol/bytebuffer.h"
#include <iostream>
#include <iomanip>
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

        // Register listening fd with event loop
        eventLoop->addFd(listenFd, [this]()
                         { handleAccept(); });

        // Register callback queue notification fd with event loop
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
                
                // Cleanup session in AuthService
                AuthService::getInstance().handleClientDisconnect(clientFd);
                
                // Cleanup rooms in RoomService
                RoomService::getInstance().handleClientDisconnect(clientFd);
                
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
                    auto task = std::make_shared<RegisterTask>(clientFd, registerReq); // Create a task (who, type)
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

                case static_cast<uint16_t>(PacketType::C2S_CreateRoom): {
                    C2S_CreateRoom createRoomReq = C2S_CreateRoom::from_payload(buf);
                    auto task = std::make_shared<CreateRoomTask>(clientFd, createRoomReq);
                    taskQueue->push(task);
                    std::cout << "Queued CreateRoomTask for client " << clientFd << std::endl;
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_LeaveRoom): {
                    C2S_LeaveRoom leaveRoomReq = C2S_LeaveRoom::from_payload(buf);
                    auto task = std::make_shared<LeaveRoomTask>(clientFd, leaveRoomReq);
                    taskQueue->push(task);
                    std::cout << "Queued LeaveRoomTask for client " << clientFd << std::endl;
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_RequestOnlineList): {
                    C2S_RequestOnlineList req = C2S_RequestOnlineList::from_payload(buf);
                    auto task = std::make_shared<RequestOnlineListTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_SendInvite): {
                    C2S_SendInvite req = C2S_SendInvite::from_payload(buf);
                    auto task = std::make_shared<SendInviteTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_RespondInvite): {
                    C2S_RespondInvite req = C2S_RespondInvite::from_payload(buf);
                    auto task = std::make_shared<RespondInviteTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_SetReady): {
                    C2S_SetReady req = C2S_SetReady::from_payload(buf);
                    auto task = std::make_shared<SetReadyTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_StartGame): {
                    C2S_StartGame req = C2S_StartGame::from_payload(buf);
                    auto task = std::make_shared<StartGameTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_KickPlayer): {
                    C2S_KickPlayer req = C2S_KickPlayer::from_payload(buf);
                    auto task = std::make_shared<KickPlayerTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_GuessChar): {
                    C2S_GuessChar req = C2S_GuessChar::from_payload(buf);
                    auto task = std::make_shared<GuessCharTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_GuessWord): {
                    C2S_GuessWord req = C2S_GuessWord::from_payload(buf);
                    auto task = std::make_shared<GuessWordTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_RequestDraw): {
                    C2S_RequestDraw req = C2S_RequestDraw::from_payload(buf);
                    auto task = std::make_shared<RequestDrawTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_EndGame): {
                    C2S_EndGame req = C2S_EndGame::from_payload(buf);
                    auto task = std::make_shared<EndGameTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_RequestSummary): {
                    C2S_RequestSummary req = C2S_RequestSummary::from_payload(buf);
                    auto task = std::make_shared<RequestSummaryTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_RequestHistory): {
                    C2S_RequestHistory req = C2S_RequestHistory::from_payload(buf);
                    auto task = std::make_shared<RequestHistoryTask>(clientFd, req);
                    taskQueue->push(task);
                    break;
                }

                case static_cast<uint16_t>(PacketType::C2S_RequestLeaderboard): {
                    C2S_RequestLeaderboard req = C2S_RequestLeaderboard::from_payload(buf);
                    auto task = std::make_shared<RequestLeaderboardTask>(clientFd, req);
                    taskQueue->push(task);
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
    // Send response được sử dụng bổi eventloop dưới sự hướng dẫn của workerthread
    void Server::sendResponse(int clientFd, const std::vector<uint8_t> &packet)
    {
        if (packet.empty()) {
            std::cerr << "WARNING: Attempt to send empty packet to client " << clientFd << std::endl;
            return;
        }
        
        auto it = connections.find(clientFd);
        if (it == connections.end())
        {
            std::cerr << "WARNING: Connection not found for client " << clientFd << std::endl;
            return;
        }

        try
        {
            // Cố  gắng gửi dữ liệu ngay lập tức
            // Nếu không thể gửi hết, dữ liệu sẽ được lưu vào bộ đệm gửi của Connection
            it->second->sendData(packet.data(), packet.size());

            // Kiểm tra liệu còn dữ liệu chưa gửi không (pendingData)
            // Nếu còn ==> đăng ký sự kiện EPOLLOUT với eventloop.
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
            TaskPtr task = taskQueue->pop(); // Wait for a task ==> Do not regiester event with eventloop
            if (!task)
            {
                break; // Queue stopped
            }

            try
            {
                std::cerr << "DEBUG: Executing task for client " << task->getClientFd() << std::endl;
                task->execute();
                std::cerr << "DEBUG: Task executed successfully" << std::endl;

                // Create callback to send response back to client
                auto callback = std::make_shared<FunctionCallback>(
                    [task, this]()
                    {
                        if (!task) {
                            std::cerr << "ERROR: Task is null in callback" << std::endl;
                            return;
                        }
                        
                        std::cerr << "DEBUG: Callback executing for client " << task->getClientFd() << std::endl;
                        
                        // 1. Send response to requester
                        int clientFd = task->getClientFd();
                        std::vector<uint8_t> packet = task->getResponsePacket();
                        
                        std::cerr << "DEBUG: Response packet size: " << packet.size() << std::endl;
                        
                        if (!packet.empty()) {
                            sendResponse(clientFd, packet);
                            std::cerr << "Sent response to client " << clientFd << std::endl;
                        }

                        // 2. Send broadcast packets (if any)
                        auto broadcasts = task->getBroadcastPackets();
                        std::cerr << "DEBUG: Broadcast packets count: " << broadcasts.size() << std::endl;
                        
                        for (const auto& p : broadcasts) {
                            int targetFd = p.first;
                            const auto& data = p.second;
                            std::cerr << "DEBUG: Broadcasting to fd=" << targetFd << ", size=" << data.size() << std::endl;
                            
                            if (!data.empty() && targetFd >= 0) {
                                sendResponse(targetFd, data);
                                std::cerr << "Broadcasted to client " << targetFd << std::endl;
                            } else {
                                std::cerr << "WARNING: Skipped invalid broadcast (fd=" << targetFd << ", size=" << data.size() << ")" << std::endl;
                            }
                        }
                        
                        std::cerr << "DEBUG: Callback finished" << std::endl;
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
