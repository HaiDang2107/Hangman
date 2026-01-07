#include "ui/InviteNotification.h"

InviteNotification::InviteNotification()
    : overlayWin(nullptr),
      fromUsername(""),
      countdown(15),
      selectedButton(InviteButton::ACCEPT),
      isActive(false) {
    
    getmaxyx(stdscr, height, width);
}

InviteNotification::~InviteNotification() {
    if (overlayWin) {
        delwin(overlayWin);
    }
}

void InviteNotification::show(const std::string& from) {
    fromUsername = from;
    countdown = 15;
    selectedButton = InviteButton::ACCEPT;
    isActive = true;
    
    int startY = (height - BOX_HEIGHT) / 2;
    int startX = (width - BOX_WIDTH) / 2;
    
    overlayWin = newwin(BOX_HEIGHT, BOX_WIDTH, startY, startX);
    keypad(overlayWin, TRUE);
    nodelay(overlayWin, TRUE);  // Non-blocking input
}

void InviteNotification::hide() {
    isActive = false;
    if (overlayWin) {
        delwin(overlayWin);
        overlayWin = nullptr;
    }
}

void InviteNotification::drawBox() {
    // Draw double border
    wattron(overlayWin, COLOR_PAIR(5) | A_BOLD);
    box(overlayWin, 0, 0);
    
    // Draw corners and title bar
    mvwaddch(overlayWin, 0, 0, '+');
    mvwaddch(overlayWin, 0, BOX_WIDTH - 1, '+');
    mvwaddch(overlayWin, BOX_HEIGHT - 1, 0, '+');
    mvwaddch(overlayWin, BOX_HEIGHT - 1, BOX_WIDTH - 1, '+');
    
    for (int x = 1; x < BOX_WIDTH - 1; x++) {
        mvwaddch(overlayWin, 0, x, '=');
        mvwaddch(overlayWin, BOX_HEIGHT - 1, x, '=');
    }
    
    for (int y = 1; y < BOX_HEIGHT - 1; y++) {
        mvwaddch(overlayWin, y, 0, '|');
        mvwaddch(overlayWin, y, BOX_WIDTH - 1, '|');
    }
    
    wattroff(overlayWin, COLOR_PAIR(5) | A_BOLD);
}

void InviteNotification::drawContent() {
    // Title
    wattron(overlayWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(overlayWin, 1, (BOX_WIDTH - 20) / 2, "CHALLENGE INVITATION");
    wattroff(overlayWin, COLOR_PAIR(5) | A_BOLD);
    
    // Divider
    wattron(overlayWin, COLOR_PAIR(3) | A_DIM);
    for (int x = 2; x < BOX_WIDTH - 2; x++) {
        mvwaddch(overlayWin, 2, x, '-');
    }
    wattroff(overlayWin, COLOR_PAIR(3) | A_DIM);
    
    // Message
    wattron(overlayWin, COLOR_PAIR(3));
    mvwprintw(overlayWin, 4, 4, "Player:");
    wattroff(overlayWin, COLOR_PAIR(3));
    
    wattron(overlayWin, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(overlayWin, 4, 13, "%s", fromUsername.c_str());
    wattroff(overlayWin, COLOR_PAIR(2) | A_BOLD);
    
    wattron(overlayWin, COLOR_PAIR(3));
    mvwprintw(overlayWin, 5, 4, "has challenged you to a Hangman match!");
    wattroff(overlayWin, COLOR_PAIR(3));
    
    // Countdown
    int countdownColor = (countdown <= 5) ? 4 : 5;
    wattron(overlayWin, COLOR_PAIR(countdownColor) | A_BOLD);
    mvwprintw(overlayWin, 7, (BOX_WIDTH - 25) / 2, "Time remaining: %2d seconds", countdown);
    wattroff(overlayWin, COLOR_PAIR(countdownColor) | A_BOLD);
}

void InviteNotification::drawButtons() {
    int buttonY = 9;
    int centerX = BOX_WIDTH / 2;
    
    // Accept button
    if (selectedButton == InviteButton::ACCEPT) {
        wattron(overlayWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
        mvwprintw(overlayWin, buttonY, centerX - 18, " >> [ ACCEPT ] << ");
        wattroff(overlayWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    } else {
        wattron(overlayWin, COLOR_PAIR(2));
        mvwprintw(overlayWin, buttonY, centerX - 18, "    [ ACCEPT ]    ");
        wattroff(overlayWin, COLOR_PAIR(2));
    }
    
    // Decline button
    if (selectedButton == InviteButton::DECLINE) {
        wattron(overlayWin, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
        mvwprintw(overlayWin, buttonY, centerX + 2, " >> [ DECLINE ] << ");
        wattroff(overlayWin, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
    } else {
        wattron(overlayWin, COLOR_PAIR(4));
        mvwprintw(overlayWin, buttonY, centerX + 2, "    [ DECLINE ]    ");
        wattroff(overlayWin, COLOR_PAIR(4));
    }
}

void InviteNotification::draw() {
    if (!isActive || !overlayWin) return;
    
    werase(overlayWin);
    drawBox();
    drawContent();
    drawButtons();
    wrefresh(overlayWin);
}

int InviteNotification::handleInput() {
    if (!isActive || !overlayWin) return 0;
    
    int ch = wgetch(overlayWin);
    
    if (ch == ERR) return 0;  // No input
    
    switch(ch) {
        case KEY_LEFT:
        case KEY_RIGHT:
        case 9:  // Tab
            selectedButton = (selectedButton == InviteButton::ACCEPT) ? 
                             InviteButton::DECLINE : InviteButton::ACCEPT;
            break;
            
        case 10:  // Enter
        case KEY_ENTER:
            if (selectedButton == InviteButton::ACCEPT) {
                return 1;  // Accept
            } else {
                return -1;  // Decline
            }
            break;
            
        case 27:  // ESC
            return -1;  // Decline
            break;
    }
    
    return 0;  // Continue
}

void InviteNotification::setCountdown(int seconds) {
    countdown = seconds;
    if (countdown <= 0) {
        hide();
    }
}