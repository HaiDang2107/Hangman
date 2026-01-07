#pragma once

#include "protocol/packets.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <atomic>

namespace hangman {

enum class PlayerState {
    FREE,       // Đang rảnh (Ở sảnh, không trong phòng)
    PREPARING,  // Đang chuẩn bị (Đã vào phòng, chưa bấm sẵn sàng)
    READY,      // Sẵn sàng (Đã bấm Ready, chờ chủ phòng Start)
    IN_GAME     // Đang trong trận chiến
};

enum class RoomState {
    WAITING,    // Đang chờ người chơi / chờ sẵn sàng
    PLAYING     // Đang trong trận đấu (không cho người khác join)
};

struct PlayerInfo {
    std::string username;
    int clientFd;
    PlayerState state = PlayerState::PREPARING;
};

struct Room {
    uint32_t id;
    std::string name;
    std::string host_username;
    std::vector<PlayerInfo> players; // List of players in the room
    RoomState state = RoomState::WAITING;

    bool allPlayersReady() const {
        if (players.size() < 2) return false;
        for (const auto& p : players) {
            if (p.username != host_username && p.state != PlayerState::READY) {
                return false;
            }
        }
        return true;
    }
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

    // Helper methods for BeforePlayService
    bool isUserInRoom(const std::string& username);
    Room* getRoom(uint32_t roomId);
    Room* getRoomByUsername(const std::string& username);
    
    // Allow BeforePlayService to access private members if needed, or expose necessary methods
    // For now, we'll expose methods.
    
    // Methods to modify room state
    void updatePlayerState(uint32_t roomId, const std::string& username, PlayerState newState);
    void updateRoomState(uint32_t roomId, RoomState newState);
    
    // Add player to room (for invite accept)
    S2C_CreateRoomResult joinRoom(uint32_t roomId, const std::string& username, int clientFd);
    
    // Kick player
    void kickPlayer(uint32_t roomId, const std::string& username);

    // Getters
    std::vector<PlayerInfo> getRoomPlayers(uint32_t roomId);
    
    // Get room name safely
    std::string getRoomName(uint32_t roomId);
    
    // Handle client disconnect - cleanup rooms by clientFd
    void handleClientDisconnect(int clientFd);

private:
    RoomService();
    ~RoomService() = default;

    std::unordered_map<uint32_t, Room> rooms;
    std::mutex roomsMutex;
    std::atomic<uint32_t> nextRoomId{1};  // Đảm bảo các thao tác đọc ghi với biến này là nguyên tử
};

} // namespace hangman
