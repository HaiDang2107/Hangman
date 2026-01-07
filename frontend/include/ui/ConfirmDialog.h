#ifndef CONFIRM_DIALOG_H
#define CONFIRM_DIALOG_H

#include <ncurses.h>
#include <string>

enum class ConfirmButton {
    YES,
    NO
};

class ConfirmDialog {
private:
    WINDOW* dialogWin;
    int height, width;
    
    // State
    std::string message;
    ConfirmButton selectedButton;
    bool isActive;
    
    // UI Constants
    static const int BOX_WIDTH = 50;
    static const int BOX_HEIGHT = 9;
    
    void drawBox();
    void drawContent();
    void drawButtons();
    
public:
    ConfirmDialog();
    ~ConfirmDialog();
    
    void show(const std::string& msg);
    void hide();
    void draw();
    int handleInput();  // Returns: 1=yes, -1=no, 0=continue
    
    bool active() const { return isActive; }
};

#endif // CONFIRM_DIALOG_H