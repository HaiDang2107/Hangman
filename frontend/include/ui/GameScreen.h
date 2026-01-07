#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <ncurses.h>
#include <string>
#include <vector>
#include <set>

struct GuessHistory {
    std::string player;
    std::string guess;
    bool correct;
    
    GuessHistory(const std::string& p, const std::string& g, bool c)
        : player(p), guess(g), correct(c) {}
};

struct PlayerScore {
    std::string username;
    int score;
    
    PlayerScore(const std::string& name = "", int s = 0)
        : username(name), score(s) {}
};

enum class GameButton {
    FORFEIT,
    QUIT
};

class GameScreen {
private:
    WINDOW* mainWin;
    int height, width;
    
    // Game state
    int currentRound;  // 1, 2, 3
    std::string secretWord;
    std::string exposedPattern;  // e.g., "_ a _ _"
    std::set<char> guessedLetters;
    std::vector<GuessHistory> recentGuesses;  // Last 5
    
    PlayerScore player1;
    PlayerScore player2;
    std::string currentUser;
    bool isMyTurn;
    
    int remainingAttempts;  // 7 max per player
    int hangmanStage;  // 0-7
    
    // Input
    std::string inputBuffer;
    std::string statusMessage;
    GameButton selectedButton;
    
    // Round scoring (based on requirements)
    int getCharPoints() const;
    int getWordCorrectPoints() const;
    int getWordWrongPenalty() const;
    
    // Drawing methods
    void drawBorder();
    void drawRoundInfo();
    void drawScores();
    void drawHangman();
    void drawSecretWord();
    void drawAlphabet();
    void drawTurnInfo();
    void drawInputArea();
    void drawButtons();
    void drawRecentGuesses();
    void drawHangmanStage(int stage);
    
    void handleInput(int ch);
    void processGuess();
    void addGuessToHistory(const std::string& player, const std::string& guess, bool correct);
    
public:
    GameScreen();
    ~GameScreen();
    
    void draw();
    int update();  // Returns: 0=continue, 1=round end, 2=game end, 3=forfeit, 4=quit
    
    void startRound(int round, const std::string& word);
    void setPlayers(const PlayerScore& p1, const PlayerScore& p2);
    void setCurrentUser(const std::string& username);
    void setMyTurn(bool isTurn);
    void updateExposedPattern(const std::string& pattern);
    void updateScores(int p1Score, int p2Score);
    void decrementAttempts();
    void incrementHangman();
    
    int getRound() const { return currentRound; }
    PlayerScore getPlayer1() const { return player1; }
    PlayerScore getPlayer2() const { return player2; }
    std::string getInputBuffer() const { return inputBuffer; }
    void clearInput() { inputBuffer = ""; statusMessage = ""; }
    
    void reset();
};

#endif // GAME_SCREEN_H