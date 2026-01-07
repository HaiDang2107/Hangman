#pragma once

#include "network/ClientSocket.h"
#include "protocol/packets.h"
#include <memory>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <queue>

namespace hangman {

// Event handlers for notifications
using InviteReceivedHandler = std::function<void(const S2C_InviteReceived&)>;
using InviteResponseHandler = std::function<void(const S2C_InviteResponse&)>;
using PlayerReadyUpdateHandler = std::function<void(const S2C_PlayerReadyUpdate&)>;
using GameStartHandler = std::function<void(const S2C_GameStart&)>;
using GuessCharResultHandler = std::function<void(const S2C_GuessCharResult&)>;
using GuessWordResultHandler = std::function<void(const S2C_GuessWordResult&)>;
using DrawRequestHandler = std::function<void(const S2C_DrawRequest&)>;
using GameEndHandler = std::function<void(const S2C_GameEnd&)>;

class GameClient {
public:
    static GameClient& getInstance();

    GameClient(const GameClient&) = delete;
    GameClient& operator=(const GameClient&) = delete;

    // Connection
    bool connect(const std::string& host = "127.0.0.1", int port = 5000);
    void disconnect();
    bool isConnected() const;

    // Authentication
    S2C_RegisterResult registerUser(const std::string& username, const std::string& password);
    S2C_LoginResult login(const std::string& username, const std::string& password);
    S2C_LogoutAck logout();

    // Room Management
    S2C_CreateRoomResult createRoom(const std::string& roomName);
    S2C_LeaveRoomAck leaveRoom(uint32_t roomId);
    
    // Online Users
    S2C_OnlineList requestOnlineList();
    
    // Invitations
    void sendInvite(const std::string& targetUsername, uint32_t roomId);
    S2C_CreateRoomResult respondInvite(const std::string& fromUsername, bool accept);
    
    // Game preparation
    S2C_Ack setReady(uint32_t roomId, bool ready);
    S2C_Ack startGame(uint32_t roomId);
    
    // Gameplay actions
    S2C_GuessCharResult guessChar(uint32_t roomId, uint32_t matchId, char ch);
    S2C_GuessWordResult guessWord(uint32_t roomId, uint32_t matchId, const std::string& word);
    void requestDraw(uint32_t roomId, uint32_t matchId);
    S2C_GameEnd endGame(uint32_t roomId, uint32_t matchId, uint8_t resultCode, const std::string& message);
    
    // Event loop management
    void startEventLoop();
    void stopEventLoop();
    
    // Register event handlers
    void setInviteReceivedHandler(InviteReceivedHandler handler) { onInviteReceived = handler; }
    void setInviteResponseHandler(InviteResponseHandler handler) { onInviteResponse = handler; }
    void setPlayerReadyUpdateHandler(PlayerReadyUpdateHandler handler) { onPlayerReadyUpdate = handler; }
    void setGameStartHandler(GameStartHandler handler) { onGameStart = handler; }
    void setGuessCharResultHandler(GuessCharResultHandler handler) { onGuessCharResult = handler; }
    void setGuessWordResultHandler(GuessWordResultHandler handler) { onGuessWordResult = handler; }
    void setDrawRequestHandler(DrawRequestHandler handler) { onDrawRequest = handler; }
    void setGameEndHandler(GameEndHandler handler) { onGameEnd = handler; }
    
    // Get current session token
    const std::string& getSessionToken() const { return sessionToken; }
    bool hasValidSession() const { return !sessionToken.empty(); }

private:
    GameClient();
    ~GameClient();

    // Send packet and receive response
    template<typename ResponseType>
    ResponseType sendAndReceive(const std::vector<uint8_t>& packet);
    
    // Event loop thread function
    void eventLoopThread();
    void handleNotification(const PacketHeader& header);

    std::unique_ptr<ClientSocket> socket;
    std::string sessionToken;
    std::mutex socketMutex;
    
    // Event loop
    std::unique_ptr<std::thread> eventThread;
    std::atomic<bool> eventLoopRunning;
    
    // Event handlers
    InviteReceivedHandler onInviteReceived;
    InviteResponseHandler onInviteResponse;
    PlayerReadyUpdateHandler onPlayerReadyUpdate;
    GameStartHandler onGameStart;
    GuessCharResultHandler onGuessCharResult;
    GuessWordResultHandler onGuessWordResult;
    DrawRequestHandler onDrawRequest;
    GameEndHandler onGameEnd;
};

} // namespace hangman
