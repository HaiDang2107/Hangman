#pragma once

#include "protocol/packets.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

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
    int clientFd = -1;
};

// Authentication Service - Singleton
class AuthService {
public:
    static AuthService& getInstance();

    AuthService(const AuthService&) = delete;
    AuthService& operator=(const AuthService&) = delete;

    bool loadDatabase(const std::string& dbPath);

    S2C_RegisterResult registerUser(const C2S_Register& request);
    S2C_LoginResult login(const C2S_Login& request, int clientFd);
    S2C_LogoutAck logout(const C2S_Logout& request);

    bool validateSession(const std::string& token, std::string& outUsername);

    std::vector<Session> getAllSessions();
    int getClientFd(const std::string& username);
    bool getSessionInfo(const std::string& token, Session& outSession);

    void updateUserStats(const std::string& username, bool isWin, uint32_t points);
    std::vector<User> getAllUsers();

private:
    AuthService() = default;
    ~AuthService() = default;

    // Database
    std::string dbPath;
    std::unordered_map<std::string, User> users;
    std::unordered_map<std::string, Session> sessions;

    std::mutex usersMutex;
    std::mutex sessionsMutex;

    // Helpers
    bool userExists(const std::string& username);
    bool verifyPassword(const std::string& username, const std::string& password);
    std::string hashPassword(const std::string& password);
    std::string generateSessionToken(const std::string& username);
    void addUser(const std::string& username, const std::string& password);
    bool saveUserToDatabase(const std::string& username, const std::string& password);
    bool saveAllUsersToDatabase();
};

} // namespace hangman
