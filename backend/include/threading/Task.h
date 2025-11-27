#pragma once

#include "protocol/packets.h"
#include "threading/TaskQueue.h"
#include <memory>
#include <cstdint>
#include <string>

namespace hangman {

// Abstract Task interface
class Task {
public:
    virtual ~Task() = default;

    // Execute the task (called by worker thread)
    virtual void execute() = 0;

    // Callback when task is done (called by network thread)
    virtual void onComplete() = 0;
};

using TaskPtr = std::shared_ptr<Task>;

// Example: Login task
class LoginTask : public Task {
public:
    LoginTask(int clientFd, const C2S_Login& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    void onComplete() override;

    // Getters for result
    const S2C_LoginResult& getResult() const { return result; }
    int getClientFd() const { return clientFd; }

private:
    int clientFd;
    C2S_Login request;
    S2C_LoginResult result;
};

// Example: Register task
class RegisterTask : public Task {
public:
    RegisterTask(int clientFd, const C2S_Register& request)
        : clientFd(clientFd), request(request) {}

    void execute() override;
    void onComplete() override;

    const S2C_RegisterResult& getResult() const { return result; }
    int getClientFd() const { return clientFd; }

private:
    int clientFd;
    C2S_Register request;
    S2C_RegisterResult result;
};

} // namespace hangman
