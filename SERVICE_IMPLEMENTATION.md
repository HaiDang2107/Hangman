# Service Layer Implementation Guide

This guide explains how to implement the service layer (business logic) for the hangman server.

## Folder Structure

Services should go in `backend/include/service/` and `backend/src/service/`:

```
backend/
├── include/service/
│   ├── AuthService.h       # Login/Register/Logout
│   ├── RoomService.h       # Room management
│   ├── GameService.h       # Game logic
│   └── LeaderboardService.h
└── src/service/
    ├── AuthService.cpp
    ├── RoomService.cpp
    ├── GameService.cpp
    └── LeaderboardService.cpp
```

## Example: AuthService

### Header (AuthService.h)
```cpp
#pragma once
#include "protocol/packets.h"
#include <string>

namespace hangman {

class AuthService {
public:
    // Login user
    S2C_LoginResult login(const std::string& username, 
                         const std::string& password);
    
    // Register new user
    S2C_RegisterResult registerUser(const std::string& username,
                                    const std::string& password);
    
    // Logout user
    S2C_LogoutAck logout(const std::string& token);
    
    // Validate session token
    bool validateToken(const std::string& token);
    
    // Get username from token
    std::string getUsernameFromToken(const std::string& token);

private:
    bool userExists(const std::string& username);
    bool verifyPassword(const std::string& username, 
                       const std::string& password);
    std::string generateToken(const std::string& username);
};

} // namespace hangman
```

## Task Integration Pattern

To create a task for a service:

```cpp
// In Task.h
class LoginTask : public Task {
public:
    LoginTask(int clientFd, const C2S_Login& request)
        : clientFd(clientFd), request(request) {}
    
    void execute() override {
        // Call service on worker thread
        service.login(request.username, request.password);
    }
    
    void onComplete() override {
        // Send response back to client on network thread
    }
};
```

## Usage Pattern in Server

### Step 1: Parse incoming packet
```cpp
void Server::processPacket(int clientFd, 
                          const uint8_t* data, 
                          size_t len) {
    ByteBuffer buf(data, data + len);
    uint16_t packetType = buf.read_u16();
    
    switch (packetType) {
        case static_cast<uint16_t>(PacketType::C2S_Login):
            handleLoginRequest(clientFd, buf);
            break;
        // ...
    }
}
```

### Step 2: Create task
```cpp
void Server::handleLoginRequest(int clientFd, ByteBuffer& buf) {
    C2S_Login login = C2S_Login::from_payload(buf);
    auto task = std::make_shared<LoginTask>(clientFd, login);
    taskQueue->push(task);
}
```

### Step 3: Task executes on worker thread
```cpp
void LoginTask::execute() {
    // This runs on worker thread - can do blocking operations
    result = AuthService::getInstance().login(
        request.username,
        request.password
    );
}
```

### Step 4: Callback sends response on network thread
```cpp
void LoginTask::onComplete() {
    // This would be called by callback
    // Connection now available on main network thread
}
```

## Session Management

### Token Storage
```cpp
// In server or AuthService
class SessionManager {
    std::map<std::string, SessionInfo> sessions; // token -> info
    std::map<int, std::string> clientTokens;     // fd -> token
};

struct SessionInfo {
    std::string username;
    uint64_t loginTime;
    uint64_t lastActivity;
};
```

### Validation
```cpp
bool isSessionValid(const std::string& token) {
    auto it = sessions.find(token);
    if (it == sessions.end()) return false;
    
    // Check timeout (e.g., 30 minutes)
    auto now = std::time(nullptr);
    if (now - it->second.lastActivity > 30 * 60) {
        sessions.erase(it);
        return false;
    }
    
    it->second.lastActivity = now;
    return true;
}
```

## Database Operations

### File Format
Current format: `username:password\n`

```
user1:pass1
user2:pass2
user3:pass3
```

### Reading
```cpp
std::vector<std::string> readUsernames() {
    std::vector<std::string> users;
    std::ifstream file("backend/database/account.txt");
    std::string line;
    
    while (std::getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            users.push_back(line.substr(0, pos));
        }
    }
    return users;
}
```

### Writing (with locking)
```cpp
#include <fcntl.h>
#include <sys/file.h>

bool addUser(const std::string& username, 
             const std::string& password) {
    int fd = open("backend/database/account.txt", 
                  O_APPEND | O_CREAT | O_WRONLY, 0644);
    if (fd < 0) return false;
    
    // Lock file
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        return false;
    }
    
    std::string entry = username + ":" + password + "\n";
    write(fd, entry.c_str(), entry.size());
    
    flock(fd, LOCK_UN);
    close(fd);
    return true;
}
```

## Error Handling

### Service Level
```cpp
class ServiceException : public std::exception {
    const char* what() const noexcept override {
        return message.c_str();
    }
private:
    std::string message;
};
```

### In Task Execution
```cpp
void LoginTask::execute() {
    try {
        result = authService.login(...);
    } catch (const ServiceException& e) {
        result.code = ResultCode::SERVER_ERROR;
        result.message = e.what();
    } catch (const std::exception& e) {
        result.code = ResultCode::SERVER_ERROR;
        result.message = "Unknown error";
    }
}
```

## Best Practices

1. **Immutability**: Services should be stateless (singleton pattern)
2. **Thread Safety**: All data structures must be thread-safe
3. **Resource Management**: Use RAII for file handles and locks
4. **Validation**: Validate all input in service methods
5. **Logging**: Log important operations for debugging
6. **Transactions**: For complex operations, ensure atomicity

## Testing

Create unit tests for services:
```cpp
#include <cassert>

void testLogin() {
    AuthService auth;
    S2C_LoginResult result = auth.login("testuser", "testpass");
    assert(result.code == ResultCode::INVALID);
}
```

## Migration from Current Tasks

Move the logic from `Task.cpp` implementations into dedicated services:

```cpp
// Current (in Task.cpp):
void LoginTask::execute() {
    std::ifstream file("backend/database/account.txt");
    // ... direct file operations
}

// Better (with service):
void LoginTask::execute() {
    result = AuthService::getInstance().login(
        request.username,
        request.password
    );
}
```

## Example Full Implementation

See the implementation pattern in the existing codebase:
- Tasks are in `backend/threading/Task.h/cpp`
- Services will follow similar structure
- Each service handles one domain (auth, rooms, game, etc.)
