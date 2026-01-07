#pragma once

#include "protocol/packets.h"
#include "threading/TaskQueue.h"
#include "service/RoomService.h" // Include here for LeaveRoomResult
#include <memory>
#include <cstdint>
#include <string>
#include <variant>

namespace hangman {

// Abstract Task interface
class Task {
public:
    virtual ~Task() = default;

    // Execute the task (called by worker thread)
    virtual void execute() = 0;

    // Get client fd (để biết người gửi là ai, phục vụ broadcast)
    virtual int getClientFd() const = 0;

    // Get serialized response packet
    virtual std::vector<uint8_t> getResponsePacket() const = 0;

    // Get broadcast packets (clientFd, packetData)
    virtual std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const {
        return {};
    }
};

using TaskPtr = std::shared_ptr<Task>;

// ============ Register Task ============
class RegisterTask : public Task {
public:
    RegisterTask(int clientFd, const C2S_Register& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;

    const S2C_RegisterResult& getResult() const { return result; }

private:
    int clientFd;
    C2S_Register request;
    S2C_RegisterResult result;
};

// ============ Login Task ============
class LoginTask : public Task {
public:
    LoginTask(int clientFd, const C2S_Login& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;

    const S2C_LoginResult& getResult() const { return result; }

private:
    int clientFd;
    C2S_Login request;
    S2C_LoginResult result;
};

// ============ Logout Task ============
class LogoutTask : public Task {
public:
    LogoutTask(int clientFd, const C2S_Logout& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;

    const S2C_LogoutAck& getResult() const { return result; }

private:
    int clientFd;
    C2S_Logout request;
    S2C_LogoutAck result;
};

// ============ Create Room Task ============
class CreateRoomTask : public Task {
public:
    CreateRoomTask(int clientFd, const C2S_CreateRoom& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;

private:
    int clientFd;
    C2S_CreateRoom request;
    S2C_CreateRoomResult result;
};

// ============ Leave Room Task ============
class LeaveRoomTask : public Task {
public:
    LeaveRoomTask(int clientFd, const C2S_LeaveRoom& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;
    std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const override;

private:
    int clientFd;
    C2S_LeaveRoom request;
    LeaveRoomResult fullResult;
};

// ============ Request Online List Task ============
class RequestOnlineListTask : public Task {
public:
    RequestOnlineListTask(int clientFd, const C2S_RequestOnlineList& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;

private:
    int clientFd;
    C2S_RequestOnlineList request;
    S2C_OnlineList result;
};

// ============ Send Invite Task ============
class SendInviteTask : public Task {
public:
    SendInviteTask(int clientFd, const C2S_SendInvite& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;
    std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const override;

private:
    int clientFd;
    C2S_SendInvite request;
    S2C_Ack result; // Ack to sender
    std::vector<std::pair<int, std::vector<uint8_t>>> broadcastPackets; // Invite to target
};

// ============ Respond Invite Task ============
class RespondInviteTask : public Task {
public:
    RespondInviteTask(int clientFd, const C2S_RespondInvite& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;
    std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const override;

private:
    int clientFd;
    C2S_RespondInvite request;
    S2C_CreateRoomResult joinResult; // If accepted, result of joining room
    bool accepted;
    std::vector<std::pair<int, std::vector<uint8_t>>> broadcastPackets; // Response to inviter
};

// ============ Set Ready Task ============
class SetReadyTask : public Task {
public:
    SetReadyTask(int clientFd, const C2S_SetReady& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;
    std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const override;

private:
    int clientFd;
    C2S_SetReady request;
    S2C_Ack result;
    std::vector<std::pair<int, std::vector<uint8_t>>> broadcastPackets;
};

// ============ Start Game Task ============
class StartGameTask : public Task {
public:
    StartGameTask(int clientFd, const C2S_StartGame& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;
    std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const override;

private:
    int clientFd;
    C2S_StartGame request;
    S2C_Ack result; // Ack to host (or error)
    std::vector<std::pair<int, std::vector<uint8_t>>> broadcastPackets; // GameStart to both
};

// ============ Kick Player Task ============
class KickPlayerTask : public Task {
public:
    KickPlayerTask(int clientFd, const C2S_KickPlayer& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;
    std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const override;

private:
    int clientFd;
    C2S_KickPlayer request;
    S2C_KickResult result;
    std::vector<std::pair<int, std::vector<uint8_t>>> broadcastPackets; // Notification to kicked player
};

// ============ Guess Char Task ============
class GuessCharTask : public Task {
public:
    GuessCharTask(int clientFd, const C2S_GuessChar& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;
    std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const override;

private:
    int clientFd;
    C2S_GuessChar request;
    S2C_GuessCharResult result;
    S2C_Error error;
    bool success;
    std::vector<std::pair<int, std::vector<uint8_t>>> broadcastPackets;
};

// ============ Guess Word Task ============
class GuessWordTask : public Task {
public:
    GuessWordTask(int clientFd, const C2S_GuessWord& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;
    std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const override;

private:
    int clientFd;
    C2S_GuessWord request;
    S2C_GuessWordResult result;
    S2C_Error error;
    bool success;
    std::vector<std::pair<int, std::vector<uint8_t>>> broadcastPackets;
};

// ============ Request Draw Task ============
class RequestDrawTask : public Task {
public:
    RequestDrawTask(int clientFd, const C2S_RequestDraw& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;
    std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const override;

private:
    int clientFd;
    C2S_RequestDraw request;
    std::vector<std::pair<int, std::vector<uint8_t>>> broadcastPackets;
};

// ============ End Game Task ============
class EndGameTask : public Task {
public:
    EndGameTask(int clientFd, const C2S_EndGame& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;
    std::vector<std::pair<int, std::vector<uint8_t>>> getBroadcastPackets() const override;

private:
    int clientFd;
    C2S_EndGame request;
    S2C_GameEnd result;
    S2C_Error error;
    bool success;
    std::vector<std::pair<int, std::vector<uint8_t>>> broadcastPackets;
};

// ============ Request History Task ============
class RequestHistoryTask : public Task {
public:
    RequestHistoryTask(int clientFd, const C2S_RequestHistory& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;

private:
    int clientFd;
    C2S_RequestHistory request;
    S2C_HistoryList result;
};

// ============ Request Leaderboard Task ============
class RequestLeaderboardTask : public Task {
public:
    RequestLeaderboardTask(int clientFd, const C2S_RequestLeaderboard& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    int getClientFd() const override { return clientFd; }
    std::vector<uint8_t> getResponsePacket() const override;

private:
    int clientFd;
    C2S_RequestLeaderboard request;
    S2C_Leaderboard result;
};

} // namespace hangman
