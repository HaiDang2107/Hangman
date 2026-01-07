#include "ui/RoundEndPopup.h"
#include <algorithm>

RoundEndPopup::RoundEndPopup()
    : popupWin(nullptr),
      round(1),
      secretWord(""),
      winner(""),
      player1Score(0),
      player2Score(0),
      reason(RoundEndReason::WORD_GUESSED),
      isActive(false) {
    
    getmaxyx(stdscr, height, width);
}

RoundEndPopup::~RoundEndPopup() {
    if (popupWin) {
        delwin(popupWin);
    }
}

void RoundEndPopup::show(int roundNum, const std::string& word, const std::string& winnerName,
                         int p1Score, int p2Score, RoundEndReason endReason) {
    round = roundNum;
    secretWord = word;
    winner = winnerName;
    player1Score = p1Score;
    player2Score = p2Score;
    reason = endReason;
    isActive = true;
    
    int startY = (height - BOX_HEIGHT) / 2;
    int startX = (width - BOX_WIDTH) / 2;
    
    popupWin = newwin(BOX_HEIGHT, BOX_WIDTH, startY, startX);
    keypad(popupWin, TRUE);
}

void RoundEndPopup::hide() {
    isActive = false;
    if (popupWin) {
        delwin(popupWin);
        popupWin = nullptr;
    }
}

void RoundEndPopup::drawBox() {
    wattron(popupWin, COLOR_PAIR(5) | A_BOLD);
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
    
    wattroff(popupWin, COLOR_PAIR(5) | A_BOLD);
}

std::string RoundEndPopup::getReasonText() const {
    switch(reason) {
        case RoundEndReason::WORD_GUESSED:
            return winner + " guessed the secret word!";
        case RoundEndReason::FORFEIT:
            return winner + " forfeited this round";
        case RoundEndReason::OUT_OF_ATTEMPTS:
            return "No one guessed the word - out of attempts";
        case RoundEndReason::ALL_LETTERS_REVEALED:
            return "All letters revealed but word not guessed";
        default:
            return "Round ended";
    }
}

void RoundEndPopup::drawContent() {
    // Title
    wattron(popupWin, COLOR_PAIR(5) | A_BOLD);
    std::string title = "ROUND " + std::to_string(round) + " ENDED";
    mvwprintw(popupWin, 1, (BOX_WIDTH - title.length()) / 2, "%s", title.c_str());
    wattroff(popupWin, COLOR_PAIR(5) | A_BOLD);
    
    // Divider
    wattron(popupWin, COLOR_PAIR(3) | A_DIM);
    for (int x = 2; x < BOX_WIDTH - 2; x++) {
        mvwaddch(popupWin, 2, x, '-');
    }
    wattroff(popupWin, COLOR_PAIR(3) | A_DIM);
    
    // Reason
    wattron(popupWin, COLOR_PAIR(3));
    std::string reasonText = getReasonText();
    mvwprintw(popupWin, 4, (BOX_WIDTH - reasonText.length()) / 2, "%s", reasonText.c_str());
    wattroff(popupWin, COLOR_PAIR(3));
    
    // Secret word - BIG and CLEAR
    wattron(popupWin, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(popupWin, 6, (BOX_WIDTH - 12) / 2, "SECRET WORD:");
    wattroff(popupWin, COLOR_PAIR(2) | A_BOLD);
    
    wattron(popupWin, COLOR_PAIR(1) | A_BOLD);
    std::string upperWord = secretWord;
    std::transform(upperWord.begin(), upperWord.end(), upperWord.begin(), ::toupper);
    mvwprintw(popupWin, 8, (BOX_WIDTH - upperWord.length()) / 2, "%s", upperWord.c_str());
    wattroff(popupWin, COLOR_PAIR(1) | A_BOLD);
    
    // Scores
    wattron(popupWin, COLOR_PAIR(5));
    mvwprintw(popupWin, 10, (BOX_WIDTH - 15) / 2, "CURRENT SCORES:");
    wattroff(popupWin, COLOR_PAIR(5));
    
    wattron(popupWin, COLOR_PAIR(3));
    mvwprintw(popupWin, 11, (BOX_WIDTH - 30) / 2, "Player 1: %d pts", player1Score);
    mvwprintw(popupWin, 12, (BOX_WIDTH - 30) / 2, "Player 2: %d pts", player2Score);
    wattroff(popupWin, COLOR_PAIR(3));
    
    // Continue prompt
    wattron(popupWin, COLOR_PAIR(2));
    mvwprintw(popupWin, 14, (BOX_WIDTH - 30) / 2, "Press any key to continue...");
    wattroff(popupWin, COLOR_PAIR(2));
}

void RoundEndPopup::draw() {
    if (!isActive || !popupWin) return;
    
    werase(popupWin);
    drawBox();
    drawContent();
    wrefresh(popupWin);
}

int RoundEndPopup::handleInput() {
    if (!isActive || !popupWin) return 0;
    
    int ch = wgetch(popupWin);
    
    if (ch != ERR) {
        return 1;  // Any key pressed, continue
    }
    
    return 0;  // Still waiting
}