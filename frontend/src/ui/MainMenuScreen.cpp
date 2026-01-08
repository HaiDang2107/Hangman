#include "ui/MainMenuScreen.h"
#include <sstream>
#include <iomanip>

MainMenuScreen::MainMenuScreen() 
    : selectedOption(MenuOption::CREATE_ROOM),
      username("Player"),
      userLevel(1),
      userWins(0),
      userLosses(0) {
    
    // Lấy kích thước màn hình
    getmaxyx(stdscr, height, width);
    
    // Tạo main window
    mainWin = newwin(height, width, 0, 0);
    keypad(mainWin, TRUE);
    wtimeout(mainWin, 100);  // 100ms timeout to check notifications
    
    // Khởi tạo colors
    init_pair(1, COLOR_CYAN, COLOR_BLACK);      // Title
    init_pair(2, COLOR_GREEN, COLOR_BLACK);     // Selected
    init_pair(3, COLOR_WHITE, COLOR_BLACK);     // Normal
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);    // Info/Stats
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);   // Decoration
    init_pair(6, COLOR_RED, COLOR_BLACK);       // Logout/Quit
}

MainMenuScreen::~MainMenuScreen() {
    if (mainWin) {
        delwin(mainWin);
    }
}

void MainMenuScreen::drawBorder() {
    // No border
}

void MainMenuScreen::drawTitle() {
    wattron(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    // ASCII Art Title
    int startY = 2;
    int startX = 4;
    mvwprintw(mainWin, startY++, startX, " __  __    _    ___ _   _   __  __ _____ _   _ _   _ ");
    mvwprintw(mainWin, startY++, startX, "|  \\/  |  / \\  |_ _| \\ | | |  \\/  | ____| \\ | | | | |");
    mvwprintw(mainWin, startY++, startX, "| |\\/| | / _ \\  | ||  \\| | | |\\/| |  _| |  \\| | | | |");
    mvwprintw(mainWin, startY++, startX, "| |  | |/ ___ \\ | || |\\  | | |  | | |___| |\\  | |_| |");
    mvwprintw(mainWin, startY++, startX, "|_|  |_/_/   \\_\\___|_| \\_| |_|  |_|_____|_| \\_|\\___/ ");
    
    wattroff(mainWin, COLOR_PAIR(1) | A_BOLD);
}

void MainMenuScreen::drawUserInfo() {
    // Player info removed
}

void MainMenuScreen::drawMenu() {
    int startY = MENU_START_Y;
    int centerX = 40;
    
    // Menu items with icons
    const char* menuItems[] = {
        "[*]  CREATE ROOM",
        "[~]  VIEW MATCH HISTORY",
        "[#]  VIEW RANKINGS",
        "[-]  LOG OUT",
        "[X]  QUIT GAME"
    };
    
    for (int i = 0; i < 5; i++) {
        MenuOption option = static_cast<MenuOption>(i);
        
        if (selectedOption == option) {
            // Highlight selected option
            if (option == MenuOption::LOGOUT || option == MenuOption::QUIT) {
                wattron(mainWin, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
            } else {
                wattron(mainWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
            }
            mvwprintw(mainWin, startY, centerX - 2, " > %-30s < ", menuItems[i]);
            
            if (option == MenuOption::LOGOUT || option == MenuOption::QUIT) {
                wattroff(mainWin, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
            } else {
                wattroff(mainWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
            }
        } else {
            // Normal option
            if (option == MenuOption::LOGOUT || option == MenuOption::QUIT) {
                wattron(mainWin, COLOR_PAIR(6));
            } else {
                wattron(mainWin, COLOR_PAIR(3));
            }
            mvwprintw(mainWin, startY, centerX, "   %-30s   ", menuItems[i]);
            
            if (option == MenuOption::LOGOUT || option == MenuOption::QUIT) {
                wattroff(mainWin, COLOR_PAIR(6));
            } else {
                wattroff(mainWin, COLOR_PAIR(3));
            }
        }
        
        startY += 2;
    }
}

void MainMenuScreen::drawInstructions() {
    int y = height - 3;
    
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    mvwprintw(mainWin, y, (width - 50) / 2, "Use UP/DOWN arrows to navigate | Enter to select");
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
}

void MainMenuScreen::drawDecoration() {
    // No decorations
    
    // Welcome message
    wattron(mainWin, COLOR_PAIR(4));
    std::string welcomeMsg = "Welcome back, " + username + "!";
    mvwprintw(mainWin, 10, (width - welcomeMsg.length()) / 2, "%s", welcomeMsg.c_str());
    wattroff(mainWin, COLOR_PAIR(4));
}

void MainMenuScreen::draw() {
    werase(mainWin);
    drawBorder();
    drawTitle();
    drawDecoration();
    drawMenu();
    drawInstructions();
    wrefresh(mainWin);
}

void MainMenuScreen::handleNavigation(int ch) {
    switch(ch) {
        case KEY_UP:
            if (selectedOption == MenuOption::CREATE_ROOM) {
                selectedOption = MenuOption::QUIT;
            } else {
                int current = static_cast<int>(selectedOption);
                selectedOption = static_cast<MenuOption>(current - 1);
            }
            break;
            
        case KEY_DOWN:
            if (selectedOption == MenuOption::QUIT) {
                selectedOption = MenuOption::CREATE_ROOM;
            } else {
                int current = static_cast<int>(selectedOption);
                selectedOption = static_cast<MenuOption>(current + 1);
            }
            break;
    }
}

int MainMenuScreen::handleInput() {
    int ch = wgetch(mainWin);
    
    if (ch == KEY_UP || ch == KEY_DOWN) {
        handleNavigation(ch);
        return 0;
    }
    
    if (ch == 10 || ch == KEY_ENTER) {
        switch(selectedOption) {
            case MenuOption::CREATE_ROOM:
                return 1;
            case MenuOption::VIEW_HISTORY:
                return 2;
            case MenuOption::VIEW_RANKINGS:
                return 3;
            case MenuOption::LOGOUT:
                return -1;
            case MenuOption::QUIT:
                return -2;
        }
    }
    
    // Keyboard shortcuts
    switch(ch) {
        case 'c':
        case 'C':
            return 1;  // Create room
        case 'h':
        case 'H':
            return 2;  // History
        case 'r':
        case 'R':
            return 3;  // Rankings
        case 'l':
        case 'L':
            return -1; // Logout
        case 'q':
        case 'Q':
        case 27:  // ESC
            return -2; // Quit
    }
    
    return 0;  // Continue
}

void MainMenuScreen::setUserInfo(const std::string& name, int level, int wins, int losses) {
    username = name;
    userLevel = level;
    userWins = wins;
    userLosses = losses;
}