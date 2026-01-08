#include "network/GameClient.h"
#include "protocol/bytebuffer.h"
#include <iostream>

namespace hangman {

static GameClient* g_gameClient = nullptr;

GameClient& GameClient::getInstance() {
    if (!g_gameClient) {
        g_gameClient = new GameClient();
    }
    return *g_gameClient;
}

GameClient::GameClient() 
    : socket(std::make_unique<ClientSocket>()), 
      eventLoopRunning(false) {}

GameClient::~GameClient() {
    stopEventLoop();
    disconnect();
}

bool GameClient::connect(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(socketMutex);
    bool result = socket->connect(host, port);
    if (result) {
        startEventLoop();
    }
    return result;
}

void GameClient::disconnect() {
    stopEventLoop();
    std::lock_guard<std::mutex> lock(socketMutex);
    sessionToken.clear();
    socket->disconnect();
}

bool GameClient::isConnected() const {
    return socket->isConnected();
}

template<typename ResponseType>
ResponseType GameClient::sendAndReceive(const std::vector<uint8_t>& packet) {
    std::lock_guard<std::mutex> lock(socketMutex);
    
    ResponseType response;
    
    // Send packet
    if (!socket->send(packet)) {
        std::cerr << "Failed to send packet" << std::endl;
        return response;
    }

    // Receive header (7 bytes)
    std::vector<uint8_t> headerData;
    if (!socket->receive(headerData, 7)) {
        std::cerr << "Failed to receive header" << std::endl;
        return response;
    }

    ByteBuffer headerBuf;
    headerBuf.buf = headerData;
    
    uint8_t version = headerBuf.read_u8();
    uint16_t packetType = headerBuf.read_u16();
    uint32_t payloadLen = headerBuf.read_u32();

    (void)version;
    (void)packetType;

    // Receive payload
    std::vector<uint8_t> payloadData;
    if (payloadLen > 0) {
        if (!socket->receive(payloadData, payloadLen)) {
            std::cerr << "Failed to receive payload" << std::endl;
            return response;
        }
    }

    // Parse response
    ByteBuffer payloadBuf;
    payloadBuf.buf = payloadData;
    
    try {
        response = ResponseType::from_payload(payloadBuf);
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse response: " << e.what() << std::endl;
    }

    return response;
}

S2C_RegisterResult GameClient::registerUser(const std::string& username, const std::string& password) {
    C2S_Register request;
    request.username = username;
    request.password = password;
    
    return sendAndReceive<S2C_RegisterResult>(request.to_bytes());
}

S2C_LoginResult GameClient::login(const std::string& username, const std::string& password) {
    C2S_Login request;
    request.username = username;
    request.password = password;
    
    auto response = sendAndReceive<S2C_LoginResult>(request.to_bytes());
    
    if (response.code == ResultCode::SUCCESS) {
        sessionToken = response.session_token;
    }
    
    return response;
}

S2C_LogoutAck GameClient::logout() {
    C2S_Logout request;
    request.session_token = sessionToken;
    
    auto response = sendAndReceive<S2C_LogoutAck>(request.to_bytes());
    
    if (response.code == ResultCode::SUCCESS) {
        sessionToken.clear();
    }
    
    return response;
}

S2C_CreateRoomResult GameClient::createRoom(const std::string& roomName) {
    C2S_CreateRoom request;
    request.session_token = sessionToken;
    request.room_name = roomName;
    
    return sendAndReceive<S2C_CreateRoomResult>(request.to_bytes());
}

S2C_LeaveRoomAck GameClient::leaveRoom(uint32_t roomId) {
    C2S_LeaveRoom request;
    request.session_token = sessionToken;
    request.room_id = roomId;
    
    return sendAndReceive<S2C_LeaveRoomAck>(request.to_bytes());
}

S2C_OnlineList GameClient::requestOnlineList() {
    C2S_RequestOnlineList request;
    request.session_token = sessionToken;
    
    return sendAndReceive<S2C_OnlineList>(request.to_bytes());
}

void GameClient::sendInvite(const std::string& targetUsername, uint32_t roomId) {
    C2S_SendInvite request;
    request.session_token = sessionToken;
    request.target_username = targetUsername;
    request.room_id = roomId;
    
    auto data = request.to_bytes();
    std::lock_guard<std::mutex> lock(socketMutex);
    socket->send(data);
    // No response expected - server will notify both parties
}

S2C_CreateRoomResult GameClient::respondInvite(const std::string& fromUsername, bool accept) {
    C2S_RespondInvite request;
    request.session_token = sessionToken;
    request.from_username = fromUsername;
    request.accept = accept;
    
    if (!accept) {
        // If declining, just send without waiting for response
        auto data = request.to_bytes();
        std::lock_guard<std::mutex> lock(socketMutex);
        socket->send(data);
        
        S2C_CreateRoomResult result;
        result.code = ResultCode::SUCCESS;
        result.message = "Declined";
        return result;
    }
    
    // If accepting, wait for S2C_CreateRoomResult (join room result)
    // Temporarily stop event loop to avoid race condition
    bool wasRunning = eventLoopRunning;
    if (wasRunning) {
        stopEventLoop();
    }
    
    auto result = sendAndReceive<S2C_CreateRoomResult>(request.to_bytes());
    
    // Restart event loop if it was running
    if (wasRunning) {
        startEventLoop();
    }
    
    return result;
}

S2C_Ack GameClient::setReady(uint32_t roomId, bool ready) {
    C2S_SetReady request;
    request.session_token = sessionToken;
    request.room_id = roomId;
    request.ready = ready;
    
    // Temporarily stop event loop to avoid race condition
    bool wasRunning = eventLoopRunning;
    if (wasRunning) {
        stopEventLoop();
    }
    
    auto result = sendAndReceive<S2C_Ack>(request.to_bytes());
    
    // Restart event loop if it was running
    if (wasRunning) {
        startEventLoop();
    }
    
    return result;
}

S2C_Ack GameClient::startGame(uint32_t roomId) {
    C2S_StartGame request;
    request.session_token = sessionToken;
    request.room_id = roomId;
    
    // Temporarily stop event loop to avoid race condition
    bool wasRunning = eventLoopRunning;
    if (wasRunning) {
        stopEventLoop();
    }
    
    auto result = sendAndReceive<S2C_Ack>(request.to_bytes());
    
    // Restart event loop if it was running
    if (wasRunning) {
        startEventLoop();
    }
    
    return result;
}

S2C_GuessCharResult GameClient::guessChar(uint32_t roomId, uint32_t matchId, char ch) {
    C2S_GuessChar request;
    request.session_token = sessionToken;
    request.room_id = roomId;
    request.match_id = matchId;
    request.ch = ch;
    
    return sendAndReceive<S2C_GuessCharResult>(request.to_bytes());
}

S2C_GuessWordResult GameClient::guessWord(uint32_t roomId, uint32_t matchId, const std::string& word) {
    C2S_GuessWord request;
    request.session_token = sessionToken;
    request.room_id = roomId;
    request.match_id = matchId;
    request.word = word;
    
    return sendAndReceive<S2C_GuessWordResult>(request.to_bytes());
}

void GameClient::requestDraw(uint32_t roomId, uint32_t matchId) {
    C2S_RequestDraw request;
    request.session_token = sessionToken;
    request.room_id = roomId;
    request.match_id = matchId;
    
    // Fire and forget - response will come as notification
    std::lock_guard<std::mutex> lock(socketMutex);
    auto packet = request.to_bytes();
    socket->send(packet.data(), packet.size());
}

S2C_GameEnd GameClient::endGame(uint32_t roomId, uint32_t matchId, uint8_t resultCode, const std::string& message) {
    C2S_EndGame request;
    request.session_token = sessionToken;
    request.room_id = roomId;
    request.match_id = matchId;
    request.result_code = resultCode;
    request.message = message;
    
    return sendAndReceive<S2C_GameEnd>(request.to_bytes());
}

void GameClient::startEventLoop() {
    if (eventLoopRunning) return;
    
    eventLoopRunning = true;
    eventThread = std::make_unique<std::thread>(&GameClient::eventLoopThread, this);
}

void GameClient::stopEventLoop() {
    if (!eventLoopRunning) return;
    
    eventLoopRunning = false;
    if (eventThread && eventThread->joinable()) {
        eventThread->join();
    }
}

void GameClient::eventLoopThread() {
    while (eventLoopRunning) {
        // Check if there's data available (with 100ms timeout)
        if (!socket->hasData(100)) {
            continue;
        }
        
        // Receive header
        std::vector<uint8_t> headerData;
        {
            std::lock_guard<std::mutex> lock(socketMutex);
            
            // Double-check data is still available (might have been consumed by sendAndReceive)
            if (!socket->hasData(0)) {
                continue;
            }
            
            if (!socket->receive(headerData, 7)) {
                std::cerr << "Event loop: Failed to receive header" << std::endl;
                break;
            }
        }
        
        PacketHeader header = PacketHeader::parse_header(headerData.data(), headerData.size());
        
        // Handle notification
        handleNotification(header);
    }
}

void GameClient::handleNotification(const PacketHeader& header) {
    // Receive payload
    std::vector<uint8_t> payload;
    {
        std::lock_guard<std::mutex> lock(socketMutex);
        if (!socket->receive(payload, header.payload_len)) {
            std::cerr << "Failed to receive notification payload" << std::endl;
            return;
        }
    }
    
    ByteBuffer bb;
    bb.buf = payload;
    
    
    switch (header.type) {
        case PacketType::S2C_InviteReceived:
            if (onInviteReceived) {
                auto packet = S2C_InviteReceived::from_payload(bb);
                onInviteReceived(packet);
            } else {
            }
            break;
            
        case PacketType::S2C_InviteResponse:
            if (onInviteResponse) {
                auto packet = S2C_InviteResponse::from_payload(bb);
                onInviteResponse(packet);
            }
            break;
            
        case PacketType::S2C_PlayerReadyUpdate:
            if (onPlayerReadyUpdate) {
                auto packet = S2C_PlayerReadyUpdate::from_payload(bb);
                onPlayerReadyUpdate(packet);
            }
            break;
            
        case PacketType::S2C_GameStart:
            if (onGameStart) {
                auto packet = S2C_GameStart::from_payload(bb);
                onGameStart(packet);
            } else {
            }
            break;
            
        case PacketType::S2C_GuessCharResult:
            if (onGuessCharResult) {
                auto packet = S2C_GuessCharResult::from_payload(bb);
                onGuessCharResult(packet);
            }
            break;
            
        case PacketType::S2C_GuessWordResult:
            if (onGuessWordResult) {
                auto packet = S2C_GuessWordResult::from_payload(bb);
                onGuessWordResult(packet);
            }
            break;
            
        case PacketType::S2C_DrawRequest:
            if (onDrawRequest) {
                auto packet = S2C_DrawRequest::from_payload(bb);
                onDrawRequest(packet);
            }
            break;
            
        case PacketType::S2C_GameEnd:
            if (onGameEnd) {
                auto packet = S2C_GameEnd::from_payload(bb);
                onGameEnd(packet);
            }
            break;
            
        case PacketType::S2C_GameSummary:
            if (onGameSummary) {
                auto packet = S2C_GameSummary::from_payload(bb);
                onGameSummary(packet);
            }
            break;
            
        default:
            // Ignore unknown packet types
            break;
    }
}

} // namespace hangman
