#include "threading/Task.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

namespace hangman {

// ============ LoginTask ============

void LoginTask::execute() {
    // Read credentials from database
    std::ifstream accountFile("backend/database/account.txt");
    
    if (!accountFile.is_open()) {
        result.code = ResultCode::SERVER_ERROR;
        result.message = "Cannot access account database";
        return;
    }

    bool found = false;
    std::string line;

    while (std::getline(accountFile, line)) {
        if (line.empty()) continue;
        
        size_t delim = line.find(':');
        if (delim == std::string::npos) continue;

        std::string storedUsername = line.substr(0, delim);
        std::string storedPassword = line.substr(delim + 1);

        if (storedUsername == request.username && storedPassword == request.password) {
            found = true;
            break;
        }
    }

    accountFile.close();

    if (found) {
        result.code = ResultCode::OK;
        result.message = "Login successful";
        // TODO: Generate actual session token
        result.session_token = "token_" + request.username + "_" + std::to_string(time(nullptr));
    } else {
        result.code = ResultCode::AUTH_FAIL;
        result.message = "Invalid username or password";
    }
}

void LoginTask::onComplete() {
    // This would be called by the network thread to send response
    // The response should be serialized and sent to the client
}

// ============ RegisterTask ============

void RegisterTask::execute() {
    // Validate input
    if (request.username.empty() || request.password.empty()) {
        result.code = ResultCode::INVALID;
        result.message = "Username and password cannot be empty";
        return;
    }

    if (request.username.size() > 64 || request.password.size() > 64) {
        result.code = ResultCode::INVALID;
        result.message = "Username or password too long";
        return;
    }

    // Check if user already exists
    std::ifstream accountFile("backend/database/account.txt");
    if (accountFile.is_open()) {
        std::string line;
        while (std::getline(accountFile, line)) {
            if (line.empty()) continue;
            
            size_t delim = line.find(':');
            if (delim == std::string::npos) continue;

            std::string storedUsername = line.substr(0, delim);
            if (storedUsername == request.username) {
                result.code = ResultCode::ALREADY;
                result.message = "Username already exists";
                accountFile.close();
                return;
            }
        }
        accountFile.close();
    }

    // Add new account
    std::ofstream outFile("backend/database/account.txt", std::ios::app);
    if (!outFile.is_open()) {
        result.code = ResultCode::SERVER_ERROR;
        result.message = "Cannot write to account database";
        return;
    }

    outFile << request.username << ":" << request.password << "\n";
    outFile.close();

    result.code = ResultCode::OK;
    result.message = "Account created successfully";
}

void RegisterTask::onComplete() {
    // This would be called by the network thread to send response
}

} // namespace hangman
