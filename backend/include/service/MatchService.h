#pragma once

#include "protocol/packets.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <mutex>

namespace hangman {

struct PlayerMatchState {
    std::string username;
    std::set<char> guessedChars;
    uint8_t remainingAttempts = 6; // Standard hangman lives
    bool finished = false;
    bool won = false;
};

struct Match {
    uint32_t matchId;
    uint32_t roomId;
    std::string word;
    std::unordered_map<std::string, PlayerMatchState> playerStates;
    bool active = true;
};

struct GuessCharResult {
    S2C_GuessCharResult resultPacket;
    bool success;
    S2C_Error errorPacket;
    int opponentFd;
    std::string guesserUsername;
};

struct GuessWordResult {
    S2C_GuessWordResult resultPacket;
    bool success;
    S2C_Error errorPacket;
    bool gameEnded; // If this guess ended the game for this player
    int opponentFd;
    std::string guesserUsername;
};

struct EndGameResult {
    S2C_GameEnd endPacket;
    bool success;
    S2C_Error errorPacket;
    int opponentFd;
};

class MatchService {
public:
    static MatchService& getInstance();

    MatchService(const MatchService&) = delete;
    MatchService& operator=(const MatchService&) = delete;

    // Initialize a match (called by BeforePlayService)
    void startMatch(uint32_t roomId, const std::vector<std::string>& players, const std::string& word);

    // Game Actions
    GuessCharResult guessChar(const C2S_GuessChar& request);
    GuessWordResult guessWord(const C2S_GuessWord& request);
    
    // Draw Request
    // Returns pair<targetFd, packet>
    std::pair<int, S2C_DrawRequest> requestDraw(const C2S_RequestDraw& request);

    // End Game (Resign or explicit end)
    EndGameResult endGame(const C2S_EndGame& request);

private:
    MatchService() = default;
    ~MatchService() = default;

    std::unordered_map<uint32_t, Match> matches; // Map roomId -> Match (Assuming 1 match per room)
    std::mutex matchesMutex;

    std::string getExposedPattern(const std::string& word, const std::set<char>& guessed);
    void saveHistory(const std::string& username, const std::string& opponent, uint8_t result, const std::string& summary);
};

} // namespace hangman
