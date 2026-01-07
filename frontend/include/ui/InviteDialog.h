#pragma once
#include <ncurses.h>
#include <string>

class InviteDialog {
private:
    WINDOW* mainWin;
    WINDOW* dialogWin;
    
    int height, width;
    std::string fromUsername;
    std::string roomName;
    bool selectedAccept;  // true = Accept, false = Decline
    
    static constexpr int DIALOG_WIDTH = 60;
    static constexpr int DIALOG_HEIGHT = 14;
    
    void drawBorder();
    void drawTitle();
    void drawMessage();
    void drawButtons();
    void drawInstructions();
    void handleNavigation(int ch);
    
public:
    InviteDialog(const std::string& from, uint32_t roomId);
    ~InviteDialog();
    
    void draw();
    int show();  // Returns: 1=Accept, -1=Decline
    
    std::string getFromUsername() const { return fromUsername; }
};
