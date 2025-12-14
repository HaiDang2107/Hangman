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

} // namespace hangman
