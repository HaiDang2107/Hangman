#include "ui/GameEndPopup.h"

GameEndPopup::GameEndPopup()
    : popupWin(nullptr),
      result(GameResult::DRAW),
      player1Score(0),
      player2Score(0),
      player1Name(""),
      player2Name(""),
      isActive(false) {
    
    getmaxyx(stdscr, height, width);
}

GameEndPopup::~GameEndPopup() {
    if (popupWin) {
        delwin(popupWin);
    }
}

void GameEndPopup::show(GameResult gameResult, const std::string& p1Name, int p1Score,
                        const std::string& p2Name, int p2Score) {
    result = gameResult;
    player1Name = p1Name;
    player1Score = p1Score;
    player2Name = p2Name;
    player2Score = p2Score;
    isActive = true;
    
    int startY = (height - BOX_HEIGHT) / 2;
    int startX = (width - BOX_WIDTH) / 2;
    
    popupWin = newwin(BOX_HEIGHT, BOX_WIDTH, startY, startX);
    keypad(popupWin, TRUE);
}

void GameEndPopup::hide() {
    isActive = false;
    if (popupWin) {
        delwin(popupWin);
        popupWin = nullptr;
    }
}

void GameEndPopup::drawBox() {
    int borderColor = (result == GameResult::WIN) ? 2 : 
                      (result == GameResult::LOSS) ? 4 : 5;
    
    wattron(popupWin, COLOR_PAIR(borderColor) | A_BOLD);
    box(popupWin, 0, 0);
    
    // Enhanced corners
    mvwaddch(popupWin, 0, 0, '+');
    mvwaddch(popupWin, 0, BOX_WIDTH - 1, '+');
    mvwaddch(popupWin, BOX_HEIGHT - 1, 0, '+');
    mvwaddch(popupWin, BOX_HEIGHT - 1, BOX_WIDTH - 1, '+');
    
    for (int x = 1; x < BOX_WIDTH - 1; x++) {
        mvwaddch(popupWin, 0, x, '=');
        mvwaddch(popupWin, BOX_HEIGHT - 1, x, '=');
    }
    
    wattroff(popupWin, COLOR_PAIR(borderColor) | A_BOLD);
}

std::string GameEndPopup::getResultText() const {
    switch(result) {
        case GameResult::WIN:
            return "CONGRATULATIONS! YOU WIN!";
        case GameResult::LOSS:
            return "GAME OVER - YOU LOST";
        case GameResult::DRAW:
            return "GAME ENDED - IT'S A DRAW!";
        default:
            return "GAME ENDED";
    }
}

void GameEndPopup::drawContent() {
    // ASCII Art Title
    int titleColor = (result == GameResult::WIN) ? 2 : 
                     (result == GameResult::LOSS) ? 4 : 5;
    
    wattron(popupWin, COLOR_PAIR(titleColor) | A_BOLD);
    
    int startY = 1;
    if (result == GameResult::WIN) {
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, "__     ______  _   _  __        _____ _   _ _ ");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, "\\ \\   / / __ \\| | | | \\ \\      / /_ _| \\ | | |");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, " \\ \\ / / |  | | | | |  \\ \\ /\\ / / | ||  \\| | |");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, "  \\ V /| |  | | |_| |   \\ V  V /  | || |\\  |_|");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, "   \\_/  \\____/ \\___/     \\_/\\_/  |___|_| \\_(_)");
    } else if (result == GameResult::LOSS) {
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, "  ____    _    __  __ _____    _____     _______ ____  ");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, " / ___|  / \\  |  \\/  | ____|  / _ \\ \\   / / ____|  _ \\ ");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, "| |  _  / _ \\ | |\\/| |  _|   | | | \\ \\ / /|  _| | |_) |");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, "| |_| |/ ___ \\| |  | | |___  | |_| |\\ V / | |___|  _ < ");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, " \\____/_/   \\_\\_|  |_|_____|  \\___/  \\_/  |_____|_| \\_\\");
    } else {
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 30) / 2, " ___  ____      ___        __");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 30) / 2, "|_ _||_  _|    / _ \\      / _|");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 30) / 2, " | |   | |    | |_| |    | |_ ");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 30) / 2, " | |   | |    |  _  |    |  _|");
        mvwprintw(popupWin, startY++, (BOX_WIDTH - 30) / 2, "|___| |___|   |_| |_|    |_|  ");
    }
    
    wattroff(popupWin, COLOR_PAIR(titleColor) | A_BOLD);
    
    startY += 2;
    
    // Divider
    wattron(popupWin, COLOR_PAIR(3) | A_DIM);
    for (int x = 2; x < BOX_WIDTH - 2; x++) {
        mvwaddch(popupWin, startY, x, '-');
    }
    wattroff(popupWin, COLOR_PAIR(3) | A_DIM);
    
    startY += 2;
    
    // Final Scores
    wattron(popupWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(popupWin, startY++, (BOX_WIDTH - 15) / 2, "FINAL SCORES:");
    wattroff(popupWin, COLOR_PAIR(5) | A_BOLD);
    
    startY++;
    
    // Player 1 score
    int p1Color = (player1Score > player2Score) ? 2 : 
                  (player1Score < player2Score) ? 4 : 5;
    wattron(popupWin, COLOR_PAIR(p1Color) | A_BOLD);
    mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, "%s: %d points", 
              player1Name.c_str(), player1Score);
    wattroff(popupWin, COLOR_PAIR(p1Color) | A_BOLD);
    
    // Player 2 score
    int p2Color = (player2Score > player1Score) ? 2 : 
                  (player2Score < player1Score) ? 4 : 5;
    wattron(popupWin, COLOR_PAIR(p2Color) | A_BOLD);
    mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, "%s: %d points", 
              player2Name.c_str(), player2Score);
    wattroff(popupWin, COLOR_PAIR(p2Color) | A_BOLD);
    
    startY += 2;
    
    // Result message
    wattron(popupWin, COLOR_PAIR(3));
    mvwprintw(popupWin, startY++, (BOX_WIDTH - 40) / 2, "Results saved to match history and rankings!");
    wattroff(popupWin, COLOR_PAIR(3));
    
    startY++;
    
    // Continue prompt
    wattron(popupWin, COLOR_PAIR(2));
    mvwprintw(popupWin, startY, (BOX_WIDTH - 35) / 2, "Press any key to return to menu...");
    wattroff(popupWin, COLOR_PAIR(2));
}

void GameEndPopup::draw() {
    if (!isActive || !popupWin) return;
    
    werase(popupWin);
    drawBox();
    drawContent();
    wrefresh(popupWin);
}

int GameEndPopup::handleInput() {
    if (!isActive || !popupWin) return 0;
    
    int ch = wgetch(popupWin);
    
    if (ch != ERR) {
        return 1;  // Any key pressed, continue
    }
    
    return 0;  // Still waiting
}