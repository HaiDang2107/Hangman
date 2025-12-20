#pragma once

#include "protocol/packets.h"
#include <vector>
#include <string>
#include <utility>

namespace hangman {

struct InviteResult {
    S2C_InviteReceived invitePacket; // To target
    int targetFd;
    S2C_Error errorPacket; // To sender (if failed)
    bool success;
};

struct RespondInviteResult {
    S2C_InviteResponse responsePacket; // To sender
    int senderFd;
    S2C_CreateRoomResult joinRoomResult; // To target (if accepted)
    bool accepted;
};

struct SetReadyResult {
    S2C_PlayerReadyUpdate updatePacket; // To host
    int hostFd;
    S2C_Ack ackPacket; // To challenger
    S2C_GameStart gameStartPacket; // To both (if game starts)
    int challengerFd;
    bool gameStarted;
};

struct StartGameResult {
    S2C_GameStart gameStartPacket; // To both
    int opponentFd;
    S2C_Error errorPacket; // To host (if failed)
    bool success;
};

struct KickResult {
    S2C_KickResult resultPacket; // To both
    int targetFd;
    bool success;
};

class BeforePlayService {
public:
    static BeforePlayService& getInstance();

    BeforePlayService(const BeforePlayService&) = delete;
    BeforePlayService& operator=(const BeforePlayService&) = delete;

    // Get Online List (Free players)
    S2C_OnlineList getOnlineList(const C2S_RequestOnlineList& request);

    // Invite Player
    InviteResult sendInvite(const C2S_SendInvite& request, int senderFd);
    RespondInviteResult respondInvite(const C2S_RespondInvite& request, int targetFd);

    // Set Ready
    // Unset Ready (Handled by setReady with ready=false)
    SetReadyResult setReady(const C2S_SetReady& request, int clientFd);
    
    // Kick Player
    KickResult kickPlayer(const C2S_KickPlayer& request, int hostFd);
    
    // Start Game (Host triggers)
    StartGameResult startGame(const C2S_StartGame& request, int hostFd);

private:
    BeforePlayService() = default;
    ~BeforePlayService() = default;
};

} // namespace hangman
