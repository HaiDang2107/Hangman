#ifndef INVITE_NOTIFICATION_H
#define INVITE_NOTIFICATION_H

#include <ncurses.h>
#include <string>

enum class InviteButton {
    ACCEPT,
    DECLINE
};

class InviteNotification {
private:
    WINDOW* overlayWin;
    int height, width;
    
    // State
    std::string fromUsername;
    int countdown;
    InviteButton selectedButton;
    bool isActive;
    
    // UI Constants
    static const int BOX_WIDTH = 60;
    static const int BOX_HEIGHT = 12;
    
    void drawBox();
    void drawContent();
    void drawButtons();
    
public:
    InviteNotification();
    ~InviteNotification();
    
    void show(const std::string& from);
    void hide();
    void draw();
    int handleInput();  // Returns: 1=accept, -1=decline, 0=continue
    
    void setCountdown(int seconds);
    bool active() const { return isActive; }
};

#endif // INVITE_NOTIFICATION_H