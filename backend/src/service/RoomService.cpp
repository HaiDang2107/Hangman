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

    result.code = ResultCode::OK;
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
        result.ackPacket.code = ResultCode::OK;
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
        result.ackPacket.code = ResultCode::OK;
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

} // namespace hangman
