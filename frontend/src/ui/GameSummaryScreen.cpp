#include "ui/GameSummaryScreen.h"
#include <ncurses.h>

namespace hangman {

GameSummaryScreen::GameSummaryScreen() 
    : returnToMenu(false) {
}

void GameSummaryScreen::show(const S2C_GameSummary& summary) {
    summaryData = summary;
    returnToMenu = false;
}

void GameSummaryScreen::run() {
    // Render and wait for user to press Enter
    while (!returnToMenu) {
        render();
        int ch = getch();
        if (ch == '\n' || ch == 10 || ch == KEY_ENTER) {
            returnToMenu = true;
        }
    }
}

void GameSummaryScreen::render() {
    clear();
    
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    
    int startY = 2;
    int startX = (maxX - 70) / 2;
    
    // Title
    attron(COLOR_PAIR(3));
    mvprintw(startY, startX + 25, "=== GAME SUMMARY ===");
    attroff(COLOR_PAIR(3));
    
    startY += 2;
    
    // Column headers
    attron(COLOR_PAIR(2));
    mvprintw(startY, startX, "Player");
    mvprintw(startY, startX + 25, "Round 1");
    mvprintw(startY, startX + 35, "Round 2");
    mvprintw(startY, startX + 45, "Round 3");
    mvprintw(startY, startX + 55, "Total");
    attroff(COLOR_PAIR(2));
    
    startY++;
    mvprintw(startY, startX, "--------------------------------------------------------------");
    startY++;
    
    // Player 1 data
    mvprintw(startY, startX, "%s", summaryData.player1_username.c_str());
    mvprintw(startY, startX + 25, "%u", summaryData.player1_round1_score);
    mvprintw(startY, startX + 35, "%u", summaryData.player1_round2_score);
    mvprintw(startY, startX + 45, "%u", summaryData.player1_round3_score);
    attron(COLOR_PAIR(3));
    mvprintw(startY, startX + 55, "%u", summaryData.player1_total_score);
    attroff(COLOR_PAIR(3));
    
    startY += 2;
    
    // Player 2 data
    mvprintw(startY, startX, "%s", summaryData.player2_username.c_str());
    mvprintw(startY, startX + 25, "%u", summaryData.player2_round1_score);
    mvprintw(startY, startX + 35, "%u", summaryData.player2_round2_score);
    mvprintw(startY, startX + 45, "%u", summaryData.player2_round3_score);
    attron(COLOR_PAIR(3));
    mvprintw(startY, startX + 55, "%u", summaryData.player2_total_score);
    attroff(COLOR_PAIR(3));
    
    startY += 2;
    mvprintw(startY, startX, "--------------------------------------------------------------");
    startY += 2;
    
    // Winner announcement
    if (summaryData.winner_username.empty()) {
        attron(COLOR_PAIR(3));
        mvprintw(startY, startX + 25, "IT'S A DRAW!");
        attroff(COLOR_PAIR(3));
    } else {
        attron(COLOR_PAIR(2));
        mvprintw(startY, startX + 20, "WINNER: %s", summaryData.winner_username.c_str());
        attroff(COLOR_PAIR(2));
    }
    
    startY += 3;
    
    // Instructions
    attron(A_DIM);
    mvprintw(startY, startX + 15, "Press ENTER to return to main menu");
    attroff(A_DIM);
    
    refresh();
}

} // namespace hangman
