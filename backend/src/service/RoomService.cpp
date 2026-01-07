#include "service/RoomService.h"
#include "service/AuthService.h"
#include <iostream>

namespace hangman {

static RoomService* g_roomService = nullptr;

RoomService& RoomService::getInstance() {
    if (!g_roomService) {
        g_roomService = new RoomService();
    }
    return *g_roomService;
}

RoomService::RoomService() {}

S2C_CreateRoomResult RoomService::createRoom(const C2S_CreateRoom& request, int clientFd) {
    S2C_CreateRoomResult result;
    std::string username;

    // Validate session
    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        result.code = ResultCode::AUTH_FAIL;
        result.message = "Invalid session";
        result.room_id = 0;
        return result;
    }

    if (request.room_name.empty()) {
        result.code = ResultCode::INVALID;
        result.message = "Room name cannot be empty";
        result.room_id = 0;
        return result;
    }

    std::lock_guard<std::mutex> lock(roomsMutex);

    // Check if user is already in a room (must check inside lock to avoid race)
    for (const auto& pair : rooms) {
        for (const auto& player : pair.second.players) {
            if (player.username == username) {
                result.code = ResultCode::INVALID;
                result.message = "You are already in a room. Please leave it first.";
                result.room_id = 0;
                return result;
            }
        }
    }

    // Create room
    uint32_t roomId = nextRoomId++;
    Room room;
    room.id = roomId;
    room.name = request.room_name;
    room.host_username = username;
    
    PlayerInfo hostInfo;
    hostInfo.username = username;
    hostInfo.clientFd = clientFd;
    room.players.push_back(hostInfo);

    rooms[roomId] = room;

    result.code = ResultCode::SUCCESS;
    result.message = "Room created successfully";
    result.room_id = roomId;

    std::cout << "Room created: " << roomId << " by " << username << std::endl;

    return result;
}

LeaveRoomResult RoomService::leaveRoom(const C2S_LeaveRoom& request, int clientFd) {
    LeaveRoomResult result;
    result.leaverFd = clientFd;
    std::string username;

    // Validate session
    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        result.ackPacket.code = ResultCode::AUTH_FAIL;
        result.ackPacket.message = "Invalid session";
        return result;
    }

    std::lock_guard<std::mutex> lock(roomsMutex);

    auto it = rooms.find(request.room_id);
    if (it == rooms.end()) {
        result.ackPacket.code = ResultCode::NOT_FOUND;
        result.ackPacket.message = "Room not found";
        return result;
    }

    Room& room = it->second;
    bool found = false;
    bool isHostLeaving = (room.host_username == username);

    for (auto playerIt = room.players.begin(); playerIt != room.players.end(); ++playerIt) {
        if (playerIt->username == username) {
            room.players.erase(playerIt);
            found = true;
            break;
        }
    }

    if (!found) {
        result.ackPacket.code = ResultCode::INVALID;
        result.ackPacket.message = "User not in room";
        return result;
    }

    // Logic for notifications
    if (isHostLeaving) {
        // 1. Send to leaver (old host)
        result.ackPacket.code = ResultCode::SUCCESS;
        result.ackPacket.message = "Rời thành công";

        // 2. Send to remaining player (if any)
        if (!room.players.empty()) {
            // Assign new host
            room.host_username = room.players[0].username;
            std::cout << "New host for room " << request.room_id << ": " << room.host_username << std::endl;

            PlayerInfo& newHost = room.players[0];
            
            S2C_PlayerLeftNotification notif;
            notif.username = username; // Who left
            notif.is_new_host = true;
            notif.message = "Chủ phòng đã rời, bạn thành chủ phòng";
            
            result.broadcastPackets.push_back({newHost.clientFd, notif});
        } else {
            // Room empty, delete it
            rooms.erase(it);
            std::cout << "Room deleted: " << request.room_id << std::endl;
        }
    } else {
        // 1. Send to leaver (guest)
        result.ackPacket.code = ResultCode::SUCCESS;
        result.ackPacket.message = "Rời thành công";

        // 2. Send to host (remaining player)
        // Find host in players list
        for (const auto& p : room.players) {
            if (p.username == room.host_username) {
                S2C_PlayerLeftNotification notif;
                notif.username = username; // Who left
                notif.is_new_host = false; // Still host
                notif.message = "Đối thủ đã rời phòng";
                
                result.broadcastPackets.push_back({p.clientFd, notif});
                break;
            }
        }
    }

    std::cout << "User " << username << " left room " << request.room_id << std::endl;

    return result;
}

bool RoomService::isUserInRoom(const std::string& username) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    for (const auto& pair : rooms) {
        for (const auto& player : pair.second.players) {
            if (player.username == username) {
                return true;
            }
        }
    }
    return false;
}

Room* RoomService::getRoom(uint32_t roomId) {
    // WARNING: Not thread safe if used without lock. 
    // Should only be used if caller guarantees safety or for read-only where race is acceptable.
    // Better to use specific methods.
    auto it = rooms.find(roomId);
    if (it != rooms.end()) return &it->second;
    return nullptr;
}

Room* RoomService::getRoomByUsername(const std::string& username) {
    // WARNING: Not thread safe.
    for (auto& pair : rooms) {
        for (const auto& player : pair.second.players) {
            if (player.username == username) {
                return &pair.second;
            }
        }
    }
    return nullptr;
}

void RoomService::updatePlayerState(uint32_t roomId, const std::string& username, PlayerState newState) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        for (auto& p : it->second.players) {
            if (p.username == username) {
                p.state = newState;
                break;
            }
        }
    }
}

void RoomService::updateRoomState(uint32_t roomId, RoomState newState) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        it->second.state = newState;
    }
}

std::vector<PlayerInfo> RoomService::getRoomPlayers(uint32_t roomId) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        return it->second.players;
    }
    return {};
}

S2C_CreateRoomResult RoomService::joinRoom(uint32_t roomId, const std::string& username, int clientFd) {
    S2C_CreateRoomResult result;
    std::lock_guard<std::mutex> lock(roomsMutex);
    
    auto it = rooms.find(roomId);
    if (it == rooms.end()) {
        result.code = ResultCode::NOT_FOUND;
        result.message = "Room not found";
        result.room_id = 0;
        return result;
    }
    
    Room& room = it->second;
    if (room.players.size() >= 2) {
        result.code = ResultCode::FAIL;
        result.message = "Room is full";
        result.room_id = 0;
        return result;
    }
    
    // Check if user is already in room
    for (const auto& p : room.players) {
        if (p.username == username) {
            result.code = ResultCode::FAIL;
            result.message = "User already in room";
            result.room_id = roomId;
            return result;
        }
    }

    PlayerInfo info;
    info.username = username;
    info.clientFd = clientFd;
    info.state = PlayerState::PREPARING;
    room.players.push_back(info);
    
    result.code = ResultCode::SUCCESS;
    result.message = "Joined room successfully";
    result.room_id = roomId;
    return result;
}

void RoomService::kickPlayer(uint32_t roomId, const std::string& username) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        auto& players = it->second.players;
        for (auto pIt = players.begin(); pIt != players.end(); ++pIt) {
            if (pIt->username == username) {
                players.erase(pIt);
                break;
            }
        }
    }
}

std::string RoomService::getRoomName(uint32_t roomId) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        return it->second.name;
    }
    return "";
}

void RoomService::handleClientDisconnect(int clientFd) {
    std::lock_guard<std::mutex> lock(roomsMutex);
    
    // Find and remove player by clientFd from all rooms
    for (auto roomIt = rooms.begin(); roomIt != rooms.end(); ) {
        Room& room = roomIt->second;
        bool playerRemoved = false;
        std::string removedUsername;
        bool wasHost = false;
        
        for (auto playerIt = room.players.begin(); playerIt != room.players.end(); ++playerIt) {
            if (playerIt->clientFd == clientFd) {
                removedUsername = playerIt->username;
                wasHost = (removedUsername == room.host_username);
                room.players.erase(playerIt);
                playerRemoved = true;
                break;
            }
        }
        
        if (playerRemoved) {
            std::cout << "Player " << removedUsername << " disconnected from room " 
                      << room.id << " (fd=" << clientFd << ")" << std::endl;
            
            if (room.players.empty()) {
                // Room is now empty, delete it
                std::cout << "Room " << room.id << " deleted (empty after disconnect)" << std::endl;
                roomIt = rooms.erase(roomIt);
            } else if (wasHost) {
                // Host disconnected, assign new host
                room.host_username = room.players[0].username;
                std::cout << "New host for room " << room.id << ": " << room.host_username << std::endl;
                ++roomIt;
            } else {
                ++roomIt;
            }
        } else {
            ++roomIt;
        }
    }
}

} // namespace hangman
