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
    uint32_t score = 0;  // Player's score
    bool finished = false;
    bool won = false;
};

struct Match {
    uint32_t matchId;
    uint32_t roomId;
    std::string currentWord;  // Current round's word
    std::string round1Word;   // Round 1 word (saved for reference)
    std::string round2Word;   // Round 2 word (saved for reference)
    uint8_t currentRound = 1; // 1 or 2
    std::set<char> revealedChars;  // All correctly guessed chars (shared between players)
    std::unordered_map<std::string, PlayerMatchState> playerStates;
    bool active = true;
};

struct GuessCharResult {
    S2C_GuessCharResult resultPacket;
    bool success;
    S2C_Error errorPacket;
    int opponentFd;
    std::string guesserUsername;
    uint32_t scoreGained = 0;  // Score earned from this guess
    std::string opponentPattern;  // Pattern for opponent (after round transition if any)
    uint32_t opponentScore;       // Opponent's score (after guess)
};

struct GuessWordResult {
    S2C_GuessWordResult resultPacket;
    bool success;
    S2C_Error errorPacket;
    bool gameEnded; // If this guess ended the game for this player
    int opponentFd;
    std::string guesserUsername;
    uint32_t scoreGained = 0;  // Score earned/lost from this guess
};

struct EndGameResult {
    S2C_GameEnd endPacket;
    bool success;
    S2C_Error errorPacket;
    int opponentFd;
};

struct MatchInfo {
    uint32_t wordLength;
    uint8_t currentRound;
};

class MatchService {
public:
    static MatchService& getInstance();

    MatchService(const MatchService&) = delete;
    MatchService& operator=(const MatchService&) = delete;

    // Initialize a match (called by BeforePlayService)
    void startMatch(uint32_t roomId, const std::vector<std::string>& players, const std::string& word);
    
    // Get match info for game start
    MatchInfo getMatchInfo(uint32_t roomId);

    // Game Actions
    GuessCharResult guessChar(const C2S_GuessChar& request);
    GuessWordResult guessWord(const C2S_GuessWord& request);
    
    // Draw Request
    // Returns pair<targetFd, packet>
    std::pair<int, S2C_DrawRequest> requestDraw(const C2S_RequestDraw& request);

    // End Game (Resign or explicit end)
    EndGameResult endGame(const C2S_EndGame& request);
    
    // Get opponent's exposed pattern after a guess
    std::string getOpponentPattern(uint32_t roomId, const std::string& guesserUsername, 
                                   char guessedChar, bool wasCorrect);
    
    // Get opponent's score
    uint32_t getOpponentScore(uint32_t roomId, const std::string& guesserUsername);

private:
    MatchService() = default;
    ~MatchService() = default;

    std::unordered_map<uint32_t, Match> matches; // Map roomId -> Match (Assuming 1 match per room)
    std::mutex matchesMutex;
    
    std::vector<std::string> round1Words;  // Words for round 1 (4-7 letters)
    std::vector<std::string> round2Words;  // Words for round 2 (8-12 letters)
    
    void loadWords();  // Load words from files
    std::string getRandomWord(uint8_t round);  // Get random word for round
    uint32_t calculateScore(uint8_t round, bool correctGuess, bool isWordGuess, char ch, const std::string& word);  // Calculate score

    std::string getExposedPattern(const std::string& word, const std::set<char>& guessed);
    void saveHistory(const std::string& username, const std::string& opponent, uint8_t result, const std::string& summary);
};

} // namespace hangman
