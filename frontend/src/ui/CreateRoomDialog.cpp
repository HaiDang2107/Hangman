#include "ui/CreateRoomDialog.h"
#include <cstring>

CreateRoomDialog::CreateRoomDialog()
    : roomName(""),
      errorMessage(""),
      isActive(true) {
    
    getmaxyx(stdscr, height, width);
    
    // Create main background window
    mainWin = newwin(height, width, 0, 0);
    
    // Create dialog window (centered)
    int dialogY = (height - DIALOG_HEIGHT) / 2;
    int dialogX = (width - DIALOG_WIDTH) / 2;
    dialogWin = newwin(DIALOG_HEIGHT, DIALOG_WIDTH, dialogY, dialogX);
    keypad(dialogWin, TRUE);
    
    // Initialize colors (reuse existing pairs)
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
}

CreateRoomDialog::~CreateRoomDialog() {
    if (dialogWin) delwin(dialogWin);
    if (mainWin) delwin(mainWin);
}

void CreateRoomDialog::drawBorder() {
    box(dialogWin, 0, 0);
}

void CreateRoomDialog::drawTitle() {
    wattron(dialogWin, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(dialogWin, 1, (DIALOG_WIDTH - 16) / 2, "CREATE NEW ROOM");
    wattroff(dialogWin, COLOR_PAIR(1) | A_BOLD);
}

void CreateRoomDialog::drawInputField() {
    wattron(dialogWin, COLOR_PAIR(3));
    mvwprintw(dialogWin, 3, 4, "Room Name:");
    wattroff(dialogWin, COLOR_PAIR(3));
    
    wattron(dialogWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(dialogWin, 4, 4, "[%-40s]", roomName.c_str());
    wattroff(dialogWin, COLOR_PAIR(5) | A_BOLD);
    
    wattron(dialogWin, COLOR_PAIR(3) | A_DIM);
    mvwprintw(dialogWin, 6, 4, "Tip: Press Enter to create, ESC to cancel");
    wattroff(dialogWin, COLOR_PAIR(3) | A_DIM);
}

void CreateRoomDialog::drawButtons() {
    int buttonY = 8;
    int centerX = DIALOG_WIDTH / 2;
    
    wattron(dialogWin, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(dialogWin, buttonY, centerX - 18, "[ CREATE ]");
    wattroff(dialogWin, COLOR_PAIR(2) | A_BOLD);
    
    wattron(dialogWin, COLOR_PAIR(4));
    mvwprintw(dialogWin, buttonY, centerX + 8, "[ CANCEL ]");
    wattroff(dialogWin, COLOR_PAIR(4));
}

void CreateRoomDialog::drawError() {
    if (!errorMessage.empty()) {
        wattron(dialogWin, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(dialogWin, DIALOG_HEIGHT - 2, (DIALOG_WIDTH - errorMessage.length()) / 2, 
                  "%s", errorMessage.c_str());
        wattroff(dialogWin, COLOR_PAIR(4) | A_BOLD);
    }
}

void CreateRoomDialog::draw() {
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
    drawInputField();
    drawButtons();
    drawError();
    wrefresh(dialogWin);
}

void CreateRoomDialog::handleInput(int ch) {
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (!roomName.empty()) {
            roomName.pop_back();
        }
    } else if (ch >= 32 && ch <= 126 && roomName.length() < 40) {
        roomName += static_cast<char>(ch);
    }
    errorMessage = "";
}

int CreateRoomDialog::show() {
    while (isActive) {
        draw();
        int ch = wgetch(dialogWin);
        
        if (ch == 27) {  // ESC - Cancel
            return -1;
        } else if (ch == 10 || ch == KEY_ENTER) {  // Enter - Create
            if (roomName.empty()) {
                setError("Room name cannot be empty!");
                continue;
            }
            if (roomName.length() < 3) {
                setError("Room name must be at least 3 characters!");
                continue;
            }
            return 1;
        } else {
            handleInput(ch);
        }
    }
    return -1;
}

void CreateRoomDialog::setError(const std::string& msg) {
    errorMessage = msg;
}

void CreateRoomDialog::reset() {
    roomName = "";
    errorMessage = "";
    isActive = true;
}
