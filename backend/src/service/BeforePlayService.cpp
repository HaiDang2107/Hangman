#include "service/BeforePlayService.h"
#include "service/AuthService.h"
#include "service/RoomService.h"
#include <iostream>
#include <algorithm>

#include "service/MatchService.h"

namespace hangman {

static BeforePlayService* g_beforePlayService = nullptr;

BeforePlayService& BeforePlayService::getInstance() {
    if (!g_beforePlayService) {
        g_beforePlayService = new BeforePlayService();
    }
    return *g_beforePlayService;
}

S2C_OnlineList BeforePlayService::getOnlineList(const C2S_RequestOnlineList& request) {
    S2C_OnlineList response;
    std::string username;

    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        return response; // Empty list on auth fail
    }

    auto sessions = AuthService::getInstance().getAllSessions();
    for (const auto& session : sessions) {
        // Filter out self
        if (session.username == username) continue;

        // Check if user is FREE (not in any room)
        if (!RoomService::getInstance().isUserInRoom(session.username)) {
            response.users.push_back(session.username);
        }
    }
    return response;
}

InviteResult BeforePlayService::sendInvite(const C2S_SendInvite& request, int senderFd) {
    InviteResult result;
    result.success = false;
    result.targetFd = -1;
    
    std::string senderUsername;
    if (!AuthService::getInstance().validateSession(request.session_token, senderUsername)) {
        result.errorPacket.message = "Invalid session";
        return result;
    }

    // Check if target is online
    int targetFd = AuthService::getInstance().getClientFd(request.target_username);
    if (targetFd == -1) {
        result.errorPacket.message = "User not online";
        return result;
    }

    // Check if target is FREE
    if (RoomService::getInstance().isUserInRoom(request.target_username)) {
        result.errorPacket.message = request.target_username + " is busy";
        return result;
    }

    // Forward invite
    result.success = true;
    result.targetFd = targetFd;
    result.invitePacket.from_username = senderUsername;
    result.invitePacket.room_id = request.room_id;

    return result;
}

RespondInviteResult BeforePlayService::respondInvite(const C2S_RespondInvite& request, int targetFd) {
    RespondInviteResult result;
    result.accepted = request.accept;
    result.senderFd = -1;

    std::string targetUsername;
    if (!AuthService::getInstance().validateSession(request.session_token, targetUsername)) {
        return result;
    }

    int senderFd = AuthService::getInstance().getClientFd(request.from_username);
    if (senderFd == -1) {
        // Sender went offline
        return result;
    }
    result.senderFd = senderFd;

    if (!request.accept) {
        result.responsePacket.to_username = request.from_username;
        result.responsePacket.accepted = false;
        result.responsePacket.message = targetUsername + " declined invite";
        return result;
    }

    // Accepted: Check room
    // We need to find the room where sender is host? Or room specified in invite?
    // The invite response doesn't carry room_id, but we assume sender is in a room.
    // Let's find the room where sender is.
    Room* room = RoomService::getInstance().getRoomByUsername(request.from_username);
    
    if (!room) {
        result.joinRoomResult.code = ResultCode::NOT_FOUND;
        result.joinRoomResult.message = "Room not found or sender left";
        
        // Tell sender anyway?
        result.responsePacket.accepted = false;
        result.responsePacket.message = "Room invalid";
        return result;
    }

    if (room->players.size() >= 2) { // Assuming max 2 players
        result.joinRoomResult.code = ResultCode::FAIL;
        result.joinRoomResult.message = "Room is full";
        
        result.responsePacket.accepted = false;
        result.responsePacket.message = "Room full";
        return result;
    }

    // Add target to room
    result.joinRoomResult = RoomService::getInstance().joinRoom(room->id, targetUsername, targetFd);
    
    if (result.joinRoomResult.code != ResultCode::SUCCESS) {
        result.responsePacket.accepted = false;
        result.responsePacket.message = result.joinRoomResult.message;
        return result;
    }

    result.responsePacket.to_username = request.from_username;
    result.responsePacket.accepted = true;
    result.responsePacket.message = targetUsername + " accepted invite";
    
    return result;
}

SetReadyResult BeforePlayService::setReady(const C2S_SetReady& request, int clientFd) {
    SetReadyResult result;
    result.gameStarted = false;
    result.hostFd = -1;
    result.challengerFd = clientFd;

    std::string username;
    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        result.ackPacket.code = ResultCode::AUTH_FAIL;
        result.ackPacket.message = "Invalid session";
        return result;
    }

    Room* room = RoomService::getInstance().getRoom(request.room_id);
    if (!room) {
        result.ackPacket.code = ResultCode::NOT_FOUND;
        result.ackPacket.message = "Room not found";
        return result;
    }

    // Check if game is already playing
    if (room->state == RoomState::PLAYING) {
        result.ackPacket.code = ResultCode::FAIL;
        result.ackPacket.message = "Game already in progress";
        return result;
    }

    // Update state
    PlayerState newState = request.ready ? PlayerState::READY : PlayerState::PREPARING;
    RoomService::getInstance().updatePlayerState(request.room_id, username, newState);

    result.ackPacket.code = ResultCode::SUCCESS;
    result.ackPacket.message = "Set ready success";

    // Notify host
    // Find host fd
    for (const auto& p : room->players) {
        if (p.username == room->host_username) {
            result.hostFd = p.clientFd;
            break;
        }
    }

    result.updatePacket.username = username;
    result.updatePacket.ready = request.ready;

    return result;
}

StartGameResult BeforePlayService::startGame(const C2S_StartGame& request, int hostFd) {
    StartGameResult result;
    result.success = false;
    result.opponentFd = -1;

    std::string username;
    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        result.errorPacket.message = "Invalid session";
        return result;
    }

    Room* room = RoomService::getInstance().getRoom(request.room_id);
    if (!room) {
        result.errorPacket.message = "Room not found";
        return result;
    }

    if (room->host_username != username) {
        result.errorPacket.message = "Only host can start game";
        return result;
    }

    // Check if opponent is ready
    bool opponentReady = false;
    std::string opponentName;
    for (const auto& p : room->players) {
        if (p.username != username) {
            if (p.state == PlayerState::READY) {
                opponentReady = true;
                opponentName = p.username;
                result.opponentFd = p.clientFd;
            } else {
                result.errorPacket.message = "Opponent not ready";
                return result;
            }
        }
    }

    if (!opponentReady) {
        result.errorPacket.message = "No opponent or not ready";
        return result;
    }

    // Start Game
    RoomService::getInstance().updateRoomState(request.room_id, RoomState::PLAYING);
    RoomService::getInstance().updatePlayerState(request.room_id, username, PlayerState::IN_GAME);
    RoomService::getInstance().updatePlayerState(request.room_id, opponentName, PlayerState::IN_GAME);

    // Generate word (mock)
    std::string word = "HANGMAN"; // TODO: Random word
    
    // Initialize Match
    std::vector<std::string> players;
    for (const auto& p : room->players) {
        players.push_back(p.username);
    }
    MatchService::getInstance().startMatch(request.room_id, players, word);

    result.success = true;
    result.gameStartPacket.room_id = request.room_id;
    result.gameStartPacket.opponent_username = opponentName; // For host
    result.gameStartPacket.word_length = word.length();

    return result;
}

KickResult BeforePlayService::kickPlayer(const C2S_KickPlayer& request, int hostFd) {
    KickResult result;
    result.success = false;
    result.targetFd = -1;

    std::string username;
    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        result.resultPacket.code = ResultCode::AUTH_FAIL;
        result.resultPacket.message = "Invalid session";
        return result;
    }

    Room* room = RoomService::getInstance().getRoom(request.room_id);
    if (!room) {
        result.resultPacket.code = ResultCode::NOT_FOUND;
        result.resultPacket.message = "Room not found";
        return result;
    }

    if (room->host_username != username) {
        result.resultPacket.code = ResultCode::FAIL;
        result.resultPacket.message = "Only host can kick";
        return result;
    }

    if (room->state == RoomState::PLAYING) {
        result.resultPacket.code = ResultCode::FAIL;
        result.resultPacket.message = "Cannot kick during game";
        return result;
    }

    // Find target
    bool found = false;
    for (const auto& p : room->players) {
        if (p.username == request.target_username) {
            result.targetFd = p.clientFd;
            found = true;
            break;
        }
    }

    if (!found) {
        result.resultPacket.code = ResultCode::NOT_FOUND;
        result.resultPacket.message = "Target not in room";
        return result;
    }

    // Remove player
    RoomService::getInstance().kickPlayer(request.room_id, request.target_username);
    
    result.success = true;
    result.resultPacket.code = ResultCode::SUCCESS;
    result.resultPacket.message = "Kick success";

    return result;
}

} // namespace hangman
