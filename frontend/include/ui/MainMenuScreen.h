#ifndef MAINMENU_SCREEN_H
#define MAINMENU_SCREEN_H

#include <ncurses.h>
#include <string>

enum class MenuOption {
    CREATE_ROOM,
    VIEW_HISTORY,
    VIEW_RANKINGS,
    LOGOUT,
    QUIT
};

class MainMenuScreen {
private:
    WINDOW* mainWin;
    WINDOW* infoWin;
    int height, width;
    
    // State
    MenuOption selectedOption;
    std::string username;
    int userLevel;
    int userWins;
    int userLosses;
    
    // UI Constants
    static const int MENU_START_Y = 12;
    static const int INFO_BOX_HEIGHT = 8;
    static const int INFO_BOX_WIDTH = 40;
    
    // Private methods
    void drawBorder();
    void drawTitle();
    void drawUserInfo();
    void drawMenu();
    void drawInstructions();
    void drawDecoration();
    
    void handleNavigation(int ch);
    
public:
    MainMenuScreen();
    ~MainMenuScreen();
    
    void draw();
    int handleInput();  // Returns: 1=create room, 2=history, 3=rankings, -1=logout, -2=quit
    
    void setUserInfo(const std::string& name, int level = 1, int wins = 0, int losses = 0);
    std::string getUsername() const { return username; }
};

#endif // MAINMENU_SCREEN_H