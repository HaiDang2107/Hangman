#include "service/AuthService.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <random>
#include <iomanip>

namespace hangman {

// Singleton instance
static AuthService* g_authService = nullptr;

// Constructor
AuthService::AuthService() {}

// Đảm bảo chỉ có 1 đối tượng được khởi tạo (Singleton)
AuthService& AuthService::getInstance() {
    if (!g_authService) {
        g_authService = new AuthService();
    }
    return *g_authService;
}

bool AuthService::loadDatabase(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(usersMutex);
    
    this->dbPath = dbPath;
    std::ifstream file(dbPath);
    
    if (!file.is_open()) {
        // Database file doesn't exist yet - create empty one
        std::ofstream newFile(dbPath);
        newFile.close();
        return true;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // Parse: username:passwordHash:wins:points
        std::istringstream iss(line);
        std::string username, passwordHash, winsStr, pointsStr;
        
        if (std::getline(iss, username, ':') &&
            std::getline(iss, passwordHash, ':') &&
            std::getline(iss, winsStr, ':') &&
            std::getline(iss, pointsStr, ':')) {
            
            User user;
            user.username = username;
            user.passwordHash = passwordHash;
            
            try {
                user.wins = std::stoul(winsStr);
                user.total_points = std::stoul(pointsStr);
            } catch (...) {
                user.wins = 0;
                user.total_points = 0;
            }

            users[username] = user;
        }
    }

    file.close();
    return true;
}

S2C_RegisterResult AuthService::registerUser(const C2S_Register& request) {
    S2C_RegisterResult result;

    // Validate input
    if (request.username.empty() || request.password.empty()) {
        result.code = ResultCode::INVALID;
        result.message = "Username and password cannot be empty";
        return result;
    }

    if (request.username.length() > 64 || request.password.length() > 64) {
        result.code = ResultCode::INVALID;
        result.message = "Username or password too long";
        return result;
    }

    // Lock for entire operation to prevent race conditions
    {
        std::lock_guard<std::mutex> lock(usersMutex);
        
        // Check if user already exists
        if (userExists(request.username)) {
            result.code = ResultCode::ALREADY;
            result.message = "Username already exists";
            return result;
        }

        // Add to in-memory database first
        User user;
        user.username = request.username;
        user.passwordHash = hashPassword(request.password);
        user.wins = 0;
        user.total_points = 0;
        users[request.username] = user;
    }

    // Save to database file (outside the lock to avoid blocking other operations)
    if (!saveUserToDatabase(request.username, request.password)) {
        // If save fails, we need to remove from in-memory map
        std::lock_guard<std::mutex> lock(usersMutex);
        users.erase(request.username);
        result.code = ResultCode::SERVER_ERROR;
        result.message = "Failed to save user to database";
        return result;
    }

    result.code = ResultCode::SUCCESS;
    result.message = "Account created successfully";
    return result;
}

S2C_LoginResult AuthService::login(const C2S_Login& request, int clientFd) {
    S2C_LoginResult result;

    // Validate input
    if (request.username.empty() || request.password.empty()) {
        result.code = ResultCode::INVALID;
        result.message = "Username and password cannot be empty";
        return result;
    }

    // Verify credentials
    bool credentialsValid = false;
    User user;
    {
        std::lock_guard<std::mutex> lock(usersMutex);
        if (!userExists(request.username)) {
            result.code = ResultCode::AUTH_FAIL;
            result.message = "Invalid username or password";
            return result;
        }

        if (!verifyPassword(request.username, request.password)) {
            result.code = ResultCode::AUTH_FAIL;
            result.message = "Invalid username or password";
            return result;
        }

        user = users[request.username];
        credentialsValid = true;
    }

    if (!credentialsValid) {
        result.code = ResultCode::AUTH_FAIL;
        result.message = "Invalid username or password";
        return result;
    }

    // Generate session token
    std::string token = generateSessionToken(request.username);

    // Create session
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        Session session;
        session.username = request.username;
        session.wins = user.wins;
        session.total_points = user.total_points;
        session.createdAt = std::time(nullptr);
        session.clientFd = clientFd;
        sessions[token] = session;
    }

    result.code = ResultCode::SUCCESS;
    result.message = "Login successful";
    result.session_token = token;
    result.num_of_wins = user.wins;
    result.total_points = user.total_points;

    return result;
}

S2C_LogoutAck AuthService::logout(const C2S_Logout& request) {
    S2C_LogoutAck result;

    // Validate token
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        auto it = sessions.find(request.session_token);
        if (it == sessions.end()) {
            result.code = ResultCode::AUTH_FAIL;
            result.message = "Invalid session token";
            return result;
        }

        // Remove session
        sessions.erase(it);
    }

    result.code = ResultCode::SUCCESS;
    result.message = "Logout successful";
    return result;
}

bool AuthService::validateSession(const std::string& token, std::string& outUsername) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = sessions.find(token);
    if (it == sessions.end()) {
        return false;
    }
    outUsername = it->second.username;
    return true;
}

bool AuthService::getSessionInfo(const std::string& token, Session& outSession) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = sessions.find(token);
    if (it == sessions.end()) {
        return false;
    }
    outSession = it->second;
    return true;
}

// Private helper methods

bool AuthService::userExists(const std::string& username) {
    // Note: Call this with usersMutex already locked
    return users.find(username) != users.end();
}

bool AuthService::verifyPassword(const std::string& username, const std::string& password) {
    // Note: Call this with usersMutex already locked
    auto it = users.find(username);
    if (it == users.end()) {
        return false;
    }
    return it->second.passwordHash == hashPassword(password);
}

void AuthService::addUser(const std::string& username, const std::string& password) {
    // Note: Call this with usersMutex already locked
    User user;
    user.username = username;
    user.passwordHash = hashPassword(password);
    user.wins = 0;
    user.total_points = 0;
    users[username] = user;
}

std::string AuthService::generateSessionToken(const std::string& username) {
    // Simple token generation: username + timestamp + random number
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 999999);

    std::ostringstream oss;
    oss << username << "_" << std::time(nullptr) << "_" << dis(gen);
    return oss.str();
}

bool AuthService::saveUserToDatabase(const std::string& username, const std::string& password) {
    try {
        std::ofstream file(dbPath, std::ios::app);
        if (!file.is_open()) {
            return false;
        }

        // Format: username:passwordHash:wins:points
        std::string passwordHash = hashPassword(password);
        file << username << ":" << passwordHash << ":0:0\n";
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

std::string AuthService::hashPassword(const std::string& password) {
    // Simple hash: just return the password as-is for now
    // TODO: In production, use bcrypt or similar
    // For now, we use a simple approach to keep it simple
    return password;
}

std::vector<Session> AuthService::getAllSessions() {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    std::vector<Session> result;
    for (const auto& pair : sessions) {
        result.push_back(pair.second);
    }
    return result;
}

int AuthService::getClientFd(const std::string& username) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    for (const auto& pair : sessions) {
        if (pair.second.username == username) {
            return pair.second.clientFd;
        }
    }
    return -1;
}

void AuthService::handleClientDisconnect(int clientFd) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    
    // Find and remove session by clientFd
    for (auto it = sessions.begin(); it != sessions.end(); ) {
        if (it->second.clientFd == clientFd) {
            std::cout << "Cleaning up session for user: " << it->second.username 
                      << " (fd=" << clientFd << ")" << std::endl;
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
}

void AuthService::updateUserStats(const std::string& username, bool isWin, uint32_t points) {
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(username);
    if (it != users.end()) {
        if (isWin) it->second.wins++;
        it->second.total_points += points;
        saveAllUsersToDatabase();
    }
}

std::vector<User> AuthService::getAllUsers() {
    std::lock_guard<std::mutex> lock(usersMutex);
    std::vector<User> result;
    for (const auto& pair : users) {
        result.push_back(pair.second);
    }
    return result;
}

bool AuthService::saveAllUsersToDatabase() {
    try {
        std::ofstream file(dbPath, std::ios::trunc); // Overwrite
        if (!file.is_open()) return false;

        for (const auto& pair : users) {
            const User& u = pair.second;
            // Format: username:passwordHash:wins:points
            file << u.username << ":" << u.passwordHash << ":" << u.wins << ":" << u.total_points << "\n";
        }
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace hangman
