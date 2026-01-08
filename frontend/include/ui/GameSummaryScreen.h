#pragma once

#include "protocol/packets.h"
#include <string>

namespace hangman {

class GameSummaryScreen {
public:
    GameSummaryScreen();
    
    void show(const S2C_GameSummary& summary);
    void run();  // Blocking call - shows screen until user presses key
    bool shouldReturnToMenu() const { return returnToMenu; }
    
private:
    void render();
    
    bool returnToMenu;
    S2C_GameSummary summaryData;
};

} // namespace hangman
