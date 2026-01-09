#ifndef PLAYSCREEN_H
#define PLAYSCREEN_H

#include <string>
#include <set>
#include <vector>
#include "network/GameClient.h"

namespace hangman {

enum class InputMode {
    NORMAL,       // Waiting for command
    GUESS_CHAR,   // About to guess a character
    GUESS_WORD,   // Typing word guess
    MENU          // Options menu
};

class PlayScreen {
private:
    // Player info
    std::string roomName;
    std::string hostUsername;
    std::string guestUsername;
    std::string currentUsername;
    bool isHost;
    uint32_t roomId;
    uint32_t matchId;
    
    // Game state
    std::string wordPattern;      // "_ _ A _ _"
    std::set<char> guessedChars;  // Set of guessed characters
    int remainingAttempts;
    int wordLength;
    uint32_t currentScore;        // Player's current score
    uint8_t currentRound;         // 1 or 2
    bool isMyTurn;
    bool gameOver;
    bool iWon;
    bool summaryReceived;     // Track if summary has been received
    int waitingForSummaryTicks;  // Counter for timeout (in halfdelay ticks)
    std::string gameMessage;
    
    // Input state
    InputMode inputMode;
    std::string wordInput;        // Buffer for word guess
    int selectedMenuOption;
    bool needsFullRedraw;         // Track if full screen redraw is needed
    
    // UI helpers
    void drawHangman(int y, int x, int wrongGuesses);
    void drawWordPattern(int y, int x);
    void drawGuessedLetters(int y, int x);
    void drawGameInfo(int y, int x);
    void drawOptionsMenu(int y, int x);
    void drawInputPrompt();
    void drawGameOverScreen();
    void initInput();  // Initialize input settings with timeout
    
public:
    PlayScreen(const std::string& roomName,
               const std::string& hostUsername,
               const std::string& guestUsername,
               const std::string& currentUsername,
               bool isHost,
               uint32_t roomId,
               uint32_t matchId,
               int wordLength);
    
    void draw();
    int handleInput();  // Returns: -1 = exit game, 0 = continue
    
    // Check and process notifications (call this in game loop)
    void processNotifications();
    
    // Game state updates
    void updateWordPattern(const std::string& pattern);
    void addGuessedChar(char ch);
    void setRemainingAttempts(int attempts);
    void setMyTurn(bool myTurn);
    void setGameOver(bool won, const std::string& message);
    void setGameMessage(const std::string& message);
    void setScore(uint32_t score);
    void setRound(uint8_t round);
    void handleRoundTransition(const std::string& newPattern);
    void setSummaryReceived(bool received);
    
    // Getters
    bool isGameOver() const { return gameOver; }
    uint32_t getRoomId() const { return roomId; }
    uint32_t getMatchId() const { return matchId; }
    uint8_t getCurrentRound() const { return currentRound; }
};

} // namespace hangman

#endif
