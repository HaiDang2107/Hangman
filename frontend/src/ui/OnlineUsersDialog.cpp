#include "ui/OnlineUsersDialog.h"
#include <cstring>

OnlineUsersDialog::OnlineUsersDialog()
    : selectedIndex(0),
      scrollOffset(0),
      errorMessage("") {
    
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

OnlineUsersDialog::~OnlineUsersDialog() {
    if (dialogWin) delwin(dialogWin);
    if (mainWin) delwin(mainWin);
}

void OnlineUsersDialog::drawBorder() {
    box(dialogWin, 0, 0);
}

void OnlineUsersDialog::drawTitle() {
    wattron(dialogWin, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(dialogWin, 1, (DIALOG_WIDTH - 24) / 2, "FREE USERS - ONLINE LIST");
    wattroff(dialogWin, COLOR_PAIR(1) | A_BOLD);
    
    wattron(dialogWin, COLOR_PAIR(3) | A_DIM);
    mvwprintw(dialogWin, 2, (DIALOG_WIDTH - 45) / 2, "(Users not currently in any room)");
    wattroff(dialogWin, COLOR_PAIR(3) | A_DIM);
}

void OnlineUsersDialog::drawUserList() {
    int startY = 4;
    int listHeight = MAX_VISIBLE_USERS;
    
    // Draw list header
    wattron(dialogWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(dialogWin, startY, 4, "%-50s", "Username");
    mvwprintw(dialogWin, startY, 56, "Status");
    wattroff(dialogWin, COLOR_PAIR(5) | A_BOLD);
    
    // Draw separator
    wattron(dialogWin, COLOR_PAIR(3));
    mvwprintw(dialogWin, startY + 1, 4, "────────────────────────────────────────────────────────────");
    wattroff(dialogWin, COLOR_PAIR(3));
    
    startY += 2;
    
    if (userList.empty()) {
        wattron(dialogWin, COLOR_PAIR(3) | A_DIM);
        mvwprintw(dialogWin, startY + 5, (DIALOG_WIDTH - 30) / 2, "No free users available");
        wattroff(dialogWin, COLOR_PAIR(3) | A_DIM);
        return;
    }
    
    // Draw user list with scrolling
    int endIndex = scrollOffset + listHeight;
    if (endIndex > (int)userList.size()) {
        endIndex = userList.size();
    }
    
    for (int i = scrollOffset; i < endIndex; i++) {
        int displayY = startY + (i - scrollOffset);
        
        if (i == selectedIndex) {
            wattron(dialogWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
            mvwprintw(dialogWin, displayY, 4, "> %-48s", userList[i].c_str());
            mvwprintw(dialogWin, displayY, 56, "Free  ");
            wattroff(dialogWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
        } else {
            wattron(dialogWin, COLOR_PAIR(3));
            mvwprintw(dialogWin, displayY, 4, "  %-48s", userList[i].c_str());
            wattroff(dialogWin, COLOR_PAIR(3));
            
            wattron(dialogWin, COLOR_PAIR(2));
            mvwprintw(dialogWin, displayY, 56, "Free  ");
            wattroff(dialogWin, COLOR_PAIR(2));
        }
    }
    
    // Draw scroll indicators
    if (userList.size() > (size_t)listHeight) {
        wattron(dialogWin, COLOR_PAIR(5));
        if (scrollOffset > 0) {
            mvwprintw(dialogWin, startY - 1, DIALOG_WIDTH - 6, "^^^");
        }
        if (endIndex < (int)userList.size()) {
            mvwprintw(dialogWin, startY + listHeight, DIALOG_WIDTH - 6, "vvv");
        }
        wattroff(dialogWin, COLOR_PAIR(5));
        
        // Show count
        wattron(dialogWin, COLOR_PAIR(3) | A_DIM);
        mvwprintw(dialogWin, startY + listHeight + 1, DIALOG_WIDTH - 15, 
                  "(%d/%d)", selectedIndex + 1, (int)userList.size());
        wattroff(dialogWin, COLOR_PAIR(3) | A_DIM);
    }
}

void OnlineUsersDialog::drawButtons() {
    int buttonY = DIALOG_HEIGHT - 4;
    int centerX = DIALOG_WIDTH / 2;
    
    wattron(dialogWin, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(dialogWin, buttonY, centerX - 20, "[ INVITE ]");
    wattroff(dialogWin, COLOR_PAIR(2) | A_BOLD);
    
    wattron(dialogWin, COLOR_PAIR(4));
    mvwprintw(dialogWin, buttonY, centerX + 10, "[ CLOSE ]");
    wattroff(dialogWin, COLOR_PAIR(4));
}

void OnlineUsersDialog::drawInstructions() {
    int y = DIALOG_HEIGHT - 2;
    
    wattron(dialogWin, COLOR_PAIR(3) | A_DIM);
    mvwprintw(dialogWin, y, (DIALOG_WIDTH - 60) / 2, 
              "UP/DOWN: Navigate | Enter: Invite | ESC/C: Close");
    wattroff(dialogWin, COLOR_PAIR(3) | A_DIM);
}

void OnlineUsersDialog::drawError() {
    if (!errorMessage.empty()) {
        wattron(dialogWin, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(dialogWin, DIALOG_HEIGHT - 6, (DIALOG_WIDTH - errorMessage.length()) / 2, 
                  "%s", errorMessage.c_str());
        wattroff(dialogWin, COLOR_PAIR(4) | A_BOLD);
    }
}

void OnlineUsersDialog::draw() {
    // Dim background
    werase(mainWin);
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            mvwaddch(mainWin, y, x, ' ');
        }
    }
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
    wrefresh(mainWin);
    
    // Draw dialog
    werase(dialogWin);
    drawBorder();
    drawTitle();
    drawUserList();
    drawButtons();
    drawInstructions();
    drawError();
    wrefresh(dialogWin);
}

void OnlineUsersDialog::handleNavigation(int ch) {
    switch(ch) {
        case KEY_UP:
            if (selectedIndex > 0) {
                selectedIndex--;
                if (selectedIndex < scrollOffset) {
                    scrollOffset = selectedIndex;
                }
            }
            errorMessage = "";
            break;
            
        case KEY_DOWN:
            if (selectedIndex < (int)userList.size() - 1) {
                selectedIndex++;
                if (selectedIndex >= scrollOffset + MAX_VISIBLE_USERS) {
                    scrollOffset = selectedIndex - MAX_VISIBLE_USERS + 1;
                }
            }
            errorMessage = "";
            break;
    }
}

int OnlineUsersDialog::show() {
    while (true) {
        draw();
        int ch = wgetch(dialogWin);
        
        if (ch == 27 || ch == 'c' || ch == 'C') {  // ESC or C - Close
            return -1;
        } else if (ch == 10 || ch == KEY_ENTER || ch == 'i' || ch == 'I') {  // Enter or I - Invite
            if (userList.empty()) {
                setError("No users available to invite!");
                continue;
            }
            return 1;
        } else if (ch == KEY_UP || ch == KEY_DOWN) {
            handleNavigation(ch);
        }
    }
    return -1;
}

void OnlineUsersDialog::setUserList(const std::vector<std::string>& users) {
    userList = users;
    selectedIndex = 0;
    scrollOffset = 0;
    errorMessage = "";
}

std::string OnlineUsersDialog::getSelectedUser() const {
    if (selectedIndex >= 0 && selectedIndex < (int)userList.size()) {
        return userList[selectedIndex];
    }
    return "";
}

void OnlineUsersDialog::setError(const std::string& msg) {
    errorMessage = msg;
}

void OnlineUsersDialog::reset() {
    userList.clear();
    selectedIndex = 0;
    scrollOffset = 0;
    errorMessage = "";
}
