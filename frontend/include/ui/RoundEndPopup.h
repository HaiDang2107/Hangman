#ifndef ROUND_END_POPUP_H
#define ROUND_END_POPUP_H

#include <ncurses.h>
#include <string>

enum class RoundEndReason {
    WORD_GUESSED,
    FORFEIT,
    OUT_OF_ATTEMPTS,
    ALL_LETTERS_REVEALED
};

class RoundEndPopup {
private:
    WINDOW* popupWin;
    int height, width;
    
    // State
    int round;
    std::string secretWord;
    std::string winner;
    int player1Score;
    int player2Score;
    RoundEndReason reason;
    bool isActive;
    
    static const int BOX_WIDTH = 70;
    static const int BOX_HEIGHT = 16;
    
    void drawBox();
    void drawContent();
    std::string getReasonText() const;
    
public:
    RoundEndPopup();
    ~RoundEndPopup();
    
    void show(int roundNum, const std::string& word, const std::string& winnerName,
              int p1Score, int p2Score, RoundEndReason endReason);
    void hide();
    void draw();
    int handleInput();  // Returns: 1=continue, 0=waiting
    
    bool active() const { return isActive; }
};

#endif // ROUND_END_POPUP_H