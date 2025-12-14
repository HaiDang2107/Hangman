#pragma once

#include "protocol/packets.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <atomic>

namespace hangman {

struct PlayerInfo {
    std::string username;
    int clientFd;
};

struct Room {
    uint32_t id;
    std::string name;
    std::string host_username;
    std::vector<PlayerInfo> players; // List of players in the room
    // Add more fields as needed (e.g., game state, max players)
};

struct LeaveRoomResult {
    S2C_LeaveRoomAck ackPacket;
    int leaverFd;
    std::vector<std::pair<int, S2C_PlayerLeftNotification>> broadcastPackets;
};

class RoomService {
public:
    static RoomService& getInstance();

    // Delete copy constructor and assignment operator
    RoomService(const RoomService&) = delete;
    RoomService& operator=(const RoomService&) = delete;

    // Service methods
    S2C_CreateRoomResult createRoom(const C2S_CreateRoom& request, int clientFd);
    LeaveRoomResult leaveRoom(const C2S_LeaveRoom& request, int clientFd);

private:
    RoomService();
    ~RoomService() = default;

    std::unordered_map<uint32_t, Room> rooms;
    std::mutex roomsMutex;
    std::atomic<uint32_t> nextRoomId{1};  // Đảm bảo các thao tác đọc ghi với biến này là nguyên tử
};

} // namespace hangman
