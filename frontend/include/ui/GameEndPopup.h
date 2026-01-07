#ifndef GAME_END_POPUP_H
#define GAME_END_POPUP_H

#include <ncurses.h>
#include <string>

enum class GameResult {
    WIN,
    LOSS,
    DRAW
};

class GameEndPopup {
private:
    WINDOW* popupWin;
    int height, width;
    
    // State
    GameResult result;
    int player1Score;
    int player2Score;
    std::string player1Name;
    std::string player2Name;
    bool isActive;
    
    static const int BOX_WIDTH = 70;
    static const int BOX_HEIGHT = 18;
    
    void drawBox();
    void drawContent();
    std::string getResultText() const;
    
public:
    GameEndPopup();
    ~GameEndPopup();
    
    void show(GameResult gameResult, const std::string& p1Name, int p1Score,
              const std::string& p2Name, int p2Score);
    void hide();
    void draw();
    int handleInput();  // Returns: 1=continue, 0=waiting
    
    bool active() const { return isActive; }
};

#endif // GAME_END_POPUP_H