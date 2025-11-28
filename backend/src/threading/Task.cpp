#include "threading/Task.h"
#include "service/AuthService.h"

namespace hangman {

// ============ RegisterTask ============

void RegisterTask::execute() {
    result = AuthService::getInstance().registerUser(request);
}

void RegisterTask::onComplete() {
    // Response will be sent by Server through CallbackQueue
}

// ============ LoginTask ============

void LoginTask::execute() {
    result = AuthService::getInstance().login(request);
}

void LoginTask::onComplete() {
    // Response will be sent by Server through CallbackQueue
}

// ============ LogoutTask ============

void LogoutTask::execute() {
    result = AuthService::getInstance().logout(request);
}

void LogoutTask::onComplete() {
    // Response will be sent by Server through CallbackQueue
}

} // namespace hangman

