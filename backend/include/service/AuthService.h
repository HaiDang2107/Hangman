#pragma once

#include "protocol/packets.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace hangman {

// User data structure
struct User {
    std::string username;
    std::string passwordHash;
    uint32_t wins = 0;
    uint32_t total_points = 0;
};

// Session data structure
struct Session {
    std::string username;
    uint32_t wins;
    uint32_t total_points;
    uint64_t createdAt;
};

// Authentication Service - Singleton pattern
class AuthService {
public:
    static AuthService& getInstance();

    // Delete copy and move constructors
    AuthService(const AuthService&) = delete;
    AuthService& operator=(const AuthService&) = delete;

    // Load database from file
    bool loadDatabase(const std::string& dbPath);

    // Registration logic
    S2C_RegisterResult registerUser(const C2S_Register& request);

    // Login logic
    S2C_LoginResult login(const C2S_Login& request);

    // Logout logic
    S2C_LogoutAck logout(const C2S_Logout& request);

    // Validate session token
    bool validateSession(const std::string& token, std::string& outUsername);

    // Get session info
    bool getSessionInfo(const std::string& token, Session& outSession);

private:
    AuthService() = default;

    // Database
    std::unordered_map<std::string, User> users;  // username -> User
    std::unordered_map<std::string, Session> sessions;  // token -> Session
    std::mutex usersMutex;
    std::mutex sessionsMutex;
    std::string dbPath;

    // Helper methods
    bool userExists(const std::string& username);
    bool verifyPassword(const std::string& username, const std::string& password);
    void addUser(const std::string& username, const std::string& password);
    std::string generateSessionToken(const std::string& username);
    bool saveUserToDatabase(const std::string& username, const std::string& password);
    std::string hashPassword(const std::string& password);  // Simple hash for now
};

} // namespace hangman
