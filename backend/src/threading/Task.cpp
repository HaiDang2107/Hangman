#include "threading/Task.h"
#include "service/AuthService.h"
#include "service/RoomService.h"

namespace hangman {

// ============ RegisterTask ============

void RegisterTask::execute() {
    result = AuthService::getInstance().registerUser(request);
}

std::vector<uint8_t> RegisterTask::getResponsePacket() const {
    return result.to_bytes();
}

// ============ LoginTask ============

void LoginTask::execute() {
    result = AuthService::getInstance().login(request);
}

std::vector<uint8_t> LoginTask::getResponsePacket() const {
    return result.to_bytes();
}

// ============ LogoutTask ============

void LogoutTask::execute() {
    result = AuthService::getInstance().logout(request);
}

std::vector<uint8_t> LogoutTask::getResponsePacket() const {
    return result.to_bytes();
}

// ============ CreateRoomTask ============

void CreateRoomTask::execute() {
    result = RoomService::getInstance().createRoom(request, clientFd);
}

std::vector<uint8_t> CreateRoomTask::getResponsePacket() const {
    return result.to_bytes();
}

// ============ LeaveRoomTask ============

void LeaveRoomTask::execute() {
    fullResult = RoomService::getInstance().leaveRoom(request, clientFd);
}

std::vector<uint8_t> LeaveRoomTask::getResponsePacket() const {
    return fullResult.ackPacket.to_bytes();
}

std::vector<std::pair<int, std::vector<uint8_t>>> LeaveRoomTask::getBroadcastPackets() const {
    std::vector<std::pair<int, std::vector<uint8_t>>> packets;
    for (const auto& item : fullResult.broadcastPackets) {
        packets.push_back({item.first, item.second.to_bytes()});
    }
    return packets;
}

} // namespace hangman

