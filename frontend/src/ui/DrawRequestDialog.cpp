#include "ui/DrawRequestDialog.h"
#include <ncurses.h>

namespace hangman {

DrawRequestDialog::DrawRequestDialog(const std::string& fromUsername)
    : fromUsername(fromUsername) {
}

bool DrawRequestDialog::show() {
    // Create dialog window
    int height = 10;
    int width = 60;
    int startY = (LINES - height) / 2;
    int startX = (COLS - width) / 2;
    
    WINDOW* dialogWin = newwin(height, width, startY, startX);
    box(dialogWin, 0, 0);
    keypad(dialogWin, TRUE);
    
    // Title
    wattron(dialogWin, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(dialogWin, 1, (width - 20) / 2, "DRAW REQUEST");
    wattroff(dialogWin, COLOR_PAIR(2) | A_BOLD);
    
    // Message
    wattron(dialogWin, COLOR_PAIR(3));
    std::string msg1 = fromUsername + " has requested a draw.";
    mvwprintw(dialogWin, 3, (width - msg1.length()) / 2, "%s", msg1.c_str());
    
    std::string msg2 = "Do you accept?";
    mvwprintw(dialogWin, 4, (width - msg2.length()) / 2, "%s", msg2.c_str());
    wattroff(dialogWin, COLOR_PAIR(3));
    
    // Options
    int selectedOption = 0;  // 0 = Accept, 1 = Decline
    
    while (true) {
        // Draw options
        if (selectedOption == 0) {
            wattron(dialogWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
            mvwprintw(dialogWin, 6, width / 2 - 20, " Accept ");
            wattroff(dialogWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
            
            wattron(dialogWin, COLOR_PAIR(4));
            mvwprintw(dialogWin, 6, width / 2 + 5, " Decline ");
            wattroff(dialogWin, COLOR_PAIR(4));
        } else {
            wattron(dialogWin, COLOR_PAIR(4));
            mvwprintw(dialogWin, 6, width / 2 - 20, " Accept ");
            wattroff(dialogWin, COLOR_PAIR(4));
            
            wattron(dialogWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
            mvwprintw(dialogWin, 6, width / 2 + 5, " Decline ");
            wattroff(dialogWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
        }
        
        // Instructions
        wattron(dialogWin, COLOR_PAIR(6));
        mvwprintw(dialogWin, 8, (width - 35) / 2, "[LEFT/RIGHT] Select  [ENTER] Confirm");
        wattroff(dialogWin, COLOR_PAIR(6));
        
        wrefresh(dialogWin);
        
        int ch = wgetch(dialogWin);
        
        switch (ch) {
            case KEY_LEFT:
                selectedOption = 0;
                break;
            case KEY_RIGHT:
                selectedOption = 1;
                break;
            case 10:  // ENTER
            case 13:
                delwin(dialogWin);
                return (selectedOption == 0);
            case 27:  // ESC - treat as decline
                delwin(dialogWin);
                return false;
        }
    }
}

} // namespace hangman
