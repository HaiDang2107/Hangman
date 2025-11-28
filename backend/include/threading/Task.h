#pragma once

#include "protocol/packets.h"
#include "threading/TaskQueue.h"
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

    // Callback when task is done (called by network thread)
    virtual void onComplete() = 0;

    // Get client fd
    virtual int getClientFd() const = 0;
};

using TaskPtr = std::shared_ptr<Task>;

// ============ Register Task ============
class RegisterTask : public Task {
public:
    RegisterTask(int clientFd, const C2S_Register& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    void onComplete() override;
    int getClientFd() const override { return clientFd; }

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
    void onComplete() override;
    int getClientFd() const override { return clientFd; }

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
    void onComplete() override;
    int getClientFd() const override { return clientFd; }

    const S2C_LogoutAck& getResult() const { return result; }

private:
    int clientFd;
    C2S_Logout request;
    S2C_LogoutAck result;
};

} // namespace hangman
