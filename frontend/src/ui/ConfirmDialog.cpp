#include "ui/ConfirmDialog.h"

ConfirmDialog::ConfirmDialog()
    : dialogWin(nullptr),
      message(""),
      selectedButton(ConfirmButton::NO),
      isActive(false) {
    
    getmaxyx(stdscr, height, width);
}

ConfirmDialog::~ConfirmDialog() {
    if (dialogWin) {
        delwin(dialogWin);
    }
}

void ConfirmDialog::show(const std::string& msg) {
    message = msg;
    selectedButton = ConfirmButton::NO;  // Default to NO for safety
    isActive = true;
    
    int startY = (height - BOX_HEIGHT) / 2;
    int startX = (width - BOX_WIDTH) / 2;
    
    dialogWin = newwin(BOX_HEIGHT, BOX_WIDTH, startY, startX);
    keypad(dialogWin, TRUE);
}

void ConfirmDialog::hide() {
    isActive = false;
    if (dialogWin) {
        delwin(dialogWin);
        dialogWin = nullptr;
    }
}

void ConfirmDialog::drawBox() {
    wattron(dialogWin, COLOR_PAIR(5) | A_BOLD);
    box(dialogWin, 0, 0);
    
    // Enhanced corners
    mvwaddch(dialogWin, 0, 0, '+');
    mvwaddch(dialogWin, 0, BOX_WIDTH - 1, '+');
    mvwaddch(dialogWin, BOX_HEIGHT - 1, 0, '+');
    mvwaddch(dialogWin, BOX_HEIGHT - 1, BOX_WIDTH - 1, '+');
    
    wattroff(dialogWin, COLOR_PAIR(5) | A_BOLD);
}

void ConfirmDialog::drawContent() {
    // Title
    wattron(dialogWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(dialogWin, 1, (BOX_WIDTH - 12) / 2, "CONFIRMATION");
    wattroff(dialogWin, COLOR_PAIR(5) | A_BOLD);
    
    // Divider
    wattron(dialogWin, COLOR_PAIR(3) | A_DIM);
    for (int x = 2; x < BOX_WIDTH - 2; x++) {
        mvwaddch(dialogWin, 2, x, '-');
    }
    wattroff(dialogWin, COLOR_PAIR(3) | A_DIM);
    
    // Message (word wrap if needed)
    wattron(dialogWin, COLOR_PAIR(3));
    
    if (message.length() <= BOX_WIDTH - 8) {
        mvwprintw(dialogWin, 4, (BOX_WIDTH - message.length()) / 2, "%s", message.c_str());
    } else {
        // Simple word wrap
        int line = 3;
        size_t start = 0;
        while (start < message.length() && line < 6) {
            size_t end = start + BOX_WIDTH - 8;
            if (end > message.length()) end = message.length();
            
            std::string part = message.substr(start, end - start);
            mvwprintw(dialogWin, line++, 4, "%s", part.c_str());
            start = end;
        }
    }
    
    wattroff(dialogWin, COLOR_PAIR(3));
}

void ConfirmDialog::drawButtons() {
    int buttonY = 6;
    int centerX = BOX_WIDTH / 2;
    
    // YES button
    if (selectedButton == ConfirmButton::YES) {
        wattron(dialogWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
        mvwprintw(dialogWin, buttonY, centerX - 15, " >> [ YES ] << ");
        wattroff(dialogWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    } else {
        wattron(dialogWin, COLOR_PAIR(2));
        mvwprintw(dialogWin, buttonY, centerX - 15, "    [ YES ]    ");
        wattroff(dialogWin, COLOR_PAIR(2));
    }
    
    // NO button
    if (selectedButton == ConfirmButton::NO) {
        wattron(dialogWin, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
        mvwprintw(dialogWin, buttonY, centerX + 2, " >> [ NO ] << ");
        wattroff(dialogWin, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
    } else {
        wattron(dialogWin, COLOR_PAIR(4));
        mvwprintw(dialogWin, buttonY, centerX + 2, "    [ NO ]    ");
        wattroff(dialogWin, COLOR_PAIR(4));
    }
}

void ConfirmDialog::draw() {
    if (!isActive || !dialogWin) return;
    
    werase(dialogWin);
    drawBox();
    drawContent();
    drawButtons();
    wrefresh(dialogWin);
}

int ConfirmDialog::handleInput() {
    if (!isActive || !dialogWin) return 0;
    
    int ch = wgetch(dialogWin);
    
    switch(ch) {
        case KEY_LEFT:
        case KEY_RIGHT:
        case 9:  // Tab
            selectedButton = (selectedButton == ConfirmButton::YES) ? 
                             ConfirmButton::NO : ConfirmButton::YES;
            break;
            
        case 10:  // Enter
        case KEY_ENTER:
            if (selectedButton == ConfirmButton::YES) {
                return 1;  // YES
            } else {
                return -1;  // NO
            }
            break;
            
        case 27:  // ESC
            return -1;  // NO (default to safe option)
            break;
            
        case 'y':
        case 'Y':
            return 1;  // YES
            break;
            
        case 'n':
        case 'N':
            return -1;  // NO
            break;
    }
    
    return 0;  // Continue
}