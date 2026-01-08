#include "ui/InviteDialog.h"
#include <cstring>

InviteDialog::InviteDialog(const std::string& from, uint32_t roomId)
    : fromUsername(from),
      roomName("Room #" + std::to_string(roomId)),
      selectedAccept(true) {
    
    getmaxyx(stdscr, height, width);
    
    // Create main background window
    mainWin = newwin(height, width, 0, 0);
    
    // Create dialog window (centered)
    int dialogY = (height - DIALOG_HEIGHT) / 2;
    int dialogX = (width - DIALOG_WIDTH) / 2;
    dialogWin = newwin(DIALOG_HEIGHT, DIALOG_WIDTH, dialogY, dialogX);
    keypad(dialogWin, TRUE);
    
    // Initialize colors
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
}

InviteDialog::~InviteDialog() {
    if (dialogWin) delwin(dialogWin);
    if (mainWin) delwin(mainWin);
}

void InviteDialog::drawBorder() {
    box(dialogWin, 0, 0);
}

void InviteDialog::drawTitle() {
    wattron(dialogWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(dialogWin, 1, (DIALOG_WIDTH - 18) / 2, "INVITATION RECEIVED");
    wattroff(dialogWin, COLOR_PAIR(5) | A_BOLD);
}

void InviteDialog::drawMessage() {
    int centerX = DIALOG_WIDTH / 2;
    
    wattron(dialogWin, COLOR_PAIR(3));
    mvwprintw(dialogWin, 3, centerX - 10, "You have been invited!");
    wattroff(dialogWin, COLOR_PAIR(3));
    
    wattron(dialogWin, COLOR_PAIR(1) | A_BOLD);
    std::string fromMsg = "From: " + fromUsername;
    mvwprintw(dialogWin, 5, (DIALOG_WIDTH - fromMsg.length()) / 2, "%s", fromMsg.c_str());
    wattroff(dialogWin, COLOR_PAIR(1) | A_BOLD);
    
    wattron(dialogWin, COLOR_PAIR(5));
    std::string roomMsg = "To join: " + roomName;
    mvwprintw(dialogWin, 6, (DIALOG_WIDTH - roomMsg.length()) / 2, "%s", roomMsg.c_str());
    wattroff(dialogWin, COLOR_PAIR(5));
    
    wattron(dialogWin, COLOR_PAIR(3) | A_DIM);
    mvwprintw(dialogWin, 8, centerX - 18, "Do you want to accept this invitation?");
    wattroff(dialogWin, COLOR_PAIR(3) | A_DIM);
}

void InviteDialog::drawButtons() {
    int buttonY = 10;
    int centerX = DIALOG_WIDTH / 2;
    
    // Accept button
    if (selectedAccept) {
        wattron(dialogWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
        mvwprintw(dialogWin, buttonY, centerX - 20, ">  [ ACCEPT ]  <");
        wattroff(dialogWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    } else {
        wattron(dialogWin, COLOR_PAIR(2));
        mvwprintw(dialogWin, buttonY, centerX - 20, "   [ ACCEPT ]   ");
        wattroff(dialogWin, COLOR_PAIR(2));
    }
    
    // Decline button
    if (!selectedAccept) {
        wattron(dialogWin, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
        mvwprintw(dialogWin, buttonY, centerX + 4, ">  [ DECLINE ]  <");
        wattroff(dialogWin, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
    } else {
        wattron(dialogWin, COLOR_PAIR(4));
        mvwprintw(dialogWin, buttonY, centerX + 4, "   [ DECLINE ]   ");
        wattroff(dialogWin, COLOR_PAIR(4));
    }
}

void InviteDialog::drawInstructions() {
    int y = DIALOG_HEIGHT - 2;
    
    wattron(dialogWin, COLOR_PAIR(3) | A_DIM);
    mvwprintw(dialogWin, y, (DIALOG_WIDTH - 45) / 2, 
              "LEFT/RIGHT: Navigate | Enter: Confirm");
    wattroff(dialogWin, COLOR_PAIR(3) | A_DIM);
}

void InviteDialog::draw() {
    // Dim background - fill with space characters
    werase(mainWin);
    wbkgd(mainWin, ' ' | COLOR_PAIR(3) | A_DIM);
    wrefresh(mainWin);
    
    // Draw dialog
    werase(dialogWin);
    drawBorder();
    drawTitle();
    drawMessage();
    drawButtons();
    drawInstructions();
    wrefresh(dialogWin);
}

void InviteDialog::handleNavigation(int ch) {
    switch(ch) {
        case KEY_LEFT:
        case KEY_RIGHT:
            selectedAccept = !selectedAccept;
            break;
    }
}

int InviteDialog::show() {
    // Beep to alert user
    beep();
    
    while (true) {
        draw();
        int ch = wgetch(dialogWin);
        
        if (ch == 10 || ch == KEY_ENTER) {  // Enter - Confirm
            return selectedAccept ? 1 : -1;
        } else if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            handleNavigation(ch);
        } else if (ch == 'a' || ch == 'A') {  // A - Accept
            selectedAccept = true;
            return 1;
        } else if (ch == 'd' || ch == 'D' || ch == 27) {  // D or ESC - Decline
            selectedAccept = false;
            return -1;
        }
    }
    return -1;
}
