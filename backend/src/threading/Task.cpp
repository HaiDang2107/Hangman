#include "threading/Task.h"
#include "service/AuthService.h"
#include "service/RoomService.h"
#include "service/BeforePlayService.h"
#include "service/MatchService.h"
#include "service/SummaryService.h"

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
    result = AuthService::getInstance().login(request, clientFd);
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

// ============ RequestOnlineListTask ============

void RequestOnlineListTask::execute() {
    result = BeforePlayService::getInstance().getOnlineList(request);
}

std::vector<uint8_t> RequestOnlineListTask::getResponsePacket() const {
    return result.to_bytes();
}

// ============ SendInviteTask ============

void SendInviteTask::execute() {
    auto res = BeforePlayService::getInstance().sendInvite(request, clientFd);
    
    if (res.success) {
        result.code = ResultCode::SUCCESS;
        result.message = "Invite sent";
        result.ack_for_type = static_cast<uint16_t>(PacketType::C2S_SendInvite);
        
        if (res.targetFd != -1) {
            broadcastPackets.push_back({res.targetFd, res.invitePacket.to_bytes()});
        }
    } else {
        result.code = ResultCode::FAIL;
        result.message = res.errorPacket.message;
        result.ack_for_type = static_cast<uint16_t>(PacketType::C2S_SendInvite);
    }
}

std::vector<uint8_t> SendInviteTask::getResponsePacket() const {
    return result.to_bytes();
}

std::vector<std::pair<int, std::vector<uint8_t>>> SendInviteTask::getBroadcastPackets() const {
    return broadcastPackets;
}

// ============ RespondInviteTask ============

void RespondInviteTask::execute() {
    auto res = BeforePlayService::getInstance().respondInvite(request, clientFd);
    accepted = request.accept;
    
    if (accepted) {
        joinResult = res.joinRoomResult;
    } else {
        joinResult.code = ResultCode::FAIL;
        joinResult.message = "Declined";
    }
    
    if (res.senderFd != -1) {
        broadcastPackets.push_back({res.senderFd, res.responsePacket.to_bytes()});
    }
}

std::vector<uint8_t> RespondInviteTask::getResponsePacket() const {
    if (accepted) {
        return joinResult.to_bytes();
    }
    return {}; 
}

std::vector<std::pair<int, std::vector<uint8_t>>> RespondInviteTask::getBroadcastPackets() const {
    return broadcastPackets;
}

// ============ SetReadyTask ============

void SetReadyTask::execute() {
    auto res = BeforePlayService::getInstance().setReady(request, clientFd);
    
    result = res.ackPacket;
    
    if (res.hostFd != -1) {
         broadcastPackets.push_back(std::make_pair(res.hostFd, res.updatePacket.to_bytes()));
    }
    
    if (res.gameStarted) {
        broadcastPackets.push_back(std::make_pair(clientFd, res.gameStartPacket.to_bytes()));
        if (res.hostFd != -1) {
            broadcastPackets.push_back(std::make_pair(res.hostFd, res.gameStartPacket.to_bytes()));
        }
    }
}

std::vector<uint8_t> SetReadyTask::getResponsePacket() const {
    return result.to_bytes();
}

std::vector<std::pair<int, std::vector<uint8_t>>> SetReadyTask::getBroadcastPackets() const {
    return broadcastPackets;
}

// ============ StartGameTask ============

void StartGameTask::execute() {
    auto res = BeforePlayService::getInstance().startGame(request, clientFd);
    
    if (res.success) {
        result.code = ResultCode::SUCCESS;
        result.message = "Game started";
        result.ack_for_type = static_cast<uint16_t>(PacketType::C2S_StartGame);
        
        broadcastPackets.push_back(std::make_pair(clientFd, res.gameStartPacket.to_bytes()));
        if (res.opponentFd != -1) {
            broadcastPackets.push_back(std::make_pair(res.opponentFd, res.gameStartPacket.to_bytes()));
        }
    } else {
        result.code = ResultCode::FAIL;
        result.message = res.errorPacket.message;
        result.ack_for_type = static_cast<uint16_t>(PacketType::C2S_StartGame);
    }
}

std::vector<uint8_t> StartGameTask::getResponsePacket() const {
    return result.to_bytes();
}

std::vector<std::pair<int, std::vector<uint8_t>>> StartGameTask::getBroadcastPackets() const {
    return broadcastPackets;
}

// ============ KickPlayerTask ============

void KickPlayerTask::execute() {
    auto res = BeforePlayService::getInstance().kickPlayer(request, clientFd);
    
    result = res.resultPacket;
    
    if (res.success && res.targetFd != -1) {
        S2C_Error error;
        error.for_type = 0;
        error.message = "You have been kicked from the room";
        broadcastPackets.push_back(std::make_pair(res.targetFd, error.to_bytes()));
    }
}

std::vector<uint8_t> KickPlayerTask::getResponsePacket() const {
    return result.to_bytes();
}

std::vector<std::pair<int, std::vector<uint8_t>>> KickPlayerTask::getBroadcastPackets() const {
    return broadcastPackets;
}

// ============ GuessCharTask ============

void GuessCharTask::execute() {
    auto res = MatchService::getInstance().guessChar(request);
    success = res.success;
    if (success) {
        result = res.resultPacket;
    } else {
        error = res.errorPacket;
    }
}

std::vector<uint8_t> GuessCharTask::getResponsePacket() const {
    if (success) return result.to_bytes();
    return error.to_bytes();
}

// ============ GuessWordTask ============

void GuessWordTask::execute() {
    auto res = MatchService::getInstance().guessWord(request);
    success = res.success;
    if (success) {
        result = res.resultPacket;
    } else {
        error = res.errorPacket;
    }
}

std::vector<uint8_t> GuessWordTask::getResponsePacket() const {
    if (success) return result.to_bytes();
    return error.to_bytes();
}

// ============ RequestDrawTask ============

void RequestDrawTask::execute() {
    auto res = MatchService::getInstance().requestDraw(request);
    if (res.first != -1) {
        broadcastPackets.push_back({res.first, res.second.to_bytes()});
    }
}

std::vector<uint8_t> RequestDrawTask::getResponsePacket() const {
    return {}; // No direct response to sender, maybe Ack? Protocol doesn't specify Ack for this.
}

std::vector<std::pair<int, std::vector<uint8_t>>> RequestDrawTask::getBroadcastPackets() const {
    return broadcastPackets;
}

// ============ EndGameTask ============

void EndGameTask::execute() {
    auto res = MatchService::getInstance().endGame(request);
    success = res.success;
    if (success) {
        result = res.endPacket;
        if (res.opponentFd != -1) {
            broadcastPackets.push_back({res.opponentFd, result.to_bytes()});
        }
    } else {
        error = res.errorPacket;
    }
}

std::vector<uint8_t> EndGameTask::getResponsePacket() const {
    if (success) return result.to_bytes();
    return error.to_bytes();
}

std::vector<std::pair<int, std::vector<uint8_t>>> EndGameTask::getBroadcastPackets() const {
    return broadcastPackets;
}

// ============ RequestHistoryTask ============

void RequestHistoryTask::execute() {
    result = SummaryService::getInstance().getHistory(request);
}

std::vector<uint8_t> RequestHistoryTask::getResponsePacket() const {
    return result.to_bytes();
}

// ============ RequestLeaderboardTask ============

void RequestLeaderboardTask::execute() {
    result = SummaryService::getInstance().getLeaderboard(request);
}

std::vector<uint8_t> RequestLeaderboardTask::getResponsePacket() const {
    return result.to_bytes();
}

} // namespace hangman

