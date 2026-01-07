#pragma once
#include <ncurses.h>
#include <string>
#include <vector>

class OnlineUsersDialog {
private:
    WINDOW* mainWin;
    WINDOW* dialogWin;
    
    int height, width;
    std::vector<std::string> userList;
    int selectedIndex;
    int scrollOffset;
    std::string errorMessage;
    
    static constexpr int DIALOG_WIDTH = 70;
    static constexpr int DIALOG_HEIGHT = 24;
    static constexpr int MAX_VISIBLE_USERS = 12;
    
    void drawBorder();
    void drawTitle();
    void drawUserList();
    void drawButtons();
    void drawInstructions();
    void drawError();
    void handleNavigation(int ch);
    
public:
    OnlineUsersDialog();
    ~OnlineUsersDialog();
    
    void draw();
    int show();  // Returns: 1=Invite selected user, -1=Cancel
    
    void setUserList(const std::vector<std::string>& users);
    std::string getSelectedUser() const;
    void setError(const std::string& msg);
    void reset();
};
