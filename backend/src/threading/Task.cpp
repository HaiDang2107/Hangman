#include "threading/Task.h"
#include "service/AuthService.h"
#include "service/RoomService.h"
#include "service/BeforePlayService.h"
#include "service/MatchService.h"
#include "service/SummaryService.h"
#include <iostream>

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
    std::cerr << "DEBUG: SendInviteTask::execute() start" << std::endl;
    InviteResult res;
    BeforePlayService::getInstance().sendInvite(request, clientFd, res);
    
    std::cerr << "DEBUG: sendInvite result - success=" << res.success 
              << ", targetFd=" << res.targetFd << std::endl;
    
    if (res.success) {
        result.code = ResultCode::SUCCESS;
        result.message = "Invite sent";
        result.ack_for_type = static_cast<uint16_t>(PacketType::C2S_SendInvite);
        
        if (res.targetFd != -1) {
            auto inviteBytes = res.invitePacket.to_bytes();
            std::cerr << "DEBUG: Invite packet size=" << inviteBytes.size() << std::endl;
            broadcastPackets.push_back({res.targetFd, inviteBytes});
            std::cerr << "DEBUG: Added broadcast packet, total=" << broadcastPackets.size() << std::endl;
        }
    } else {
        result.code = ResultCode::FAIL;
        result.message = res.errorPacket.message;
        result.ack_for_type = static_cast<uint16_t>(PacketType::C2S_SendInvite);
        std::cerr << "DEBUG: Invite failed: " << res.errorPacket.message << std::endl;
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
        
        // Broadcast to opponent about the guess (only if opponent is different from requester)
        if (res.opponentFd != -1 && res.opponentFd != clientFd) {
            // Use pre-calculated opponent pattern and score from result
            S2C_GuessCharResult opponentNotif;
            opponentNotif.correct = res.resultPacket.correct;
            opponentNotif.exposed_pattern = res.opponentPattern;  // Already calculated after round transition
            opponentNotif.remaining_attempts = res.resultPacket.remaining_attempts;
            opponentNotif.score_gained = res.resultPacket.score_gained;
            opponentNotif.total_score = res.opponentScore;  // Opponent's own score
            opponentNotif.current_round = res.resultPacket.current_round;
            opponentNotif.is_your_turn = !res.resultPacket.is_your_turn;  // Opposite of guesser's turn
            
            broadcastPackets.push_back({res.opponentFd, opponentNotif.to_bytes()});
        }
    } else {
        error = res.errorPacket;
    }
}

std::vector<uint8_t> GuessCharTask::getResponsePacket() const {
    if (success) return result.to_bytes();
    return error.to_bytes();
}

std::vector<std::pair<int, std::vector<uint8_t>>> GuessCharTask::getBroadcastPackets() const {
    return broadcastPackets;
}

// ============ GuessWordTask ============

void GuessWordTask::execute() {
    auto res = MatchService::getInstance().guessWord(request);
    success = res.success;
    if (success) {
        result = res.resultPacket;
        
        // Broadcast to opponent about the guess (only if opponent is different from requester)
        if (res.opponentFd != -1 && res.opponentFd != clientFd) {
            // Get opponent's own score (not the guesser's score!)
            uint32_t opponentScore = MatchService::getInstance().getOpponentScore(
                request.room_id, res.guesserUsername
            );
            
            S2C_GuessWordResult opponentNotif;
            opponentNotif.correct = res.resultPacket.correct;
            opponentNotif.message = res.resultPacket.message;
            opponentNotif.remaining_attempts = res.resultPacket.remaining_attempts;
            opponentNotif.score_gained = res.resultPacket.score_gained;
            opponentNotif.total_score = opponentScore;  // Use opponent's score!
            opponentNotif.current_round = res.resultPacket.current_round;
            opponentNotif.round_complete = res.resultPacket.round_complete;
            opponentNotif.next_word_pattern = res.resultPacket.next_word_pattern;
            opponentNotif.is_your_turn = !res.resultPacket.is_your_turn;  // Opposite of guesser's turn
            
            broadcastPackets.push_back({res.opponentFd, opponentNotif.to_bytes()});
        }
    } else {
        error = res.errorPacket;
    }
}

std::vector<uint8_t> GuessWordTask::getResponsePacket() const {
    if (success) return result.to_bytes();
    return error.to_bytes();
}

std::vector<std::pair<int, std::vector<uint8_t>>> GuessWordTask::getBroadcastPackets() const {
    return broadcastPackets;
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

// ============ RequestSummaryTask ============

void RequestSummaryTask::execute() {
    result = MatchService::getInstance().requestSummary(request);
}

std::vector<uint8_t> RequestSummaryTask::getResponsePacket() const {
    return result.to_bytes();
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

