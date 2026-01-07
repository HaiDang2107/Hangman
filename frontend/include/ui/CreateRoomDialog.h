#pragma once
#include <ncurses.h>
#include <string>

class CreateRoomDialog {
private:
    WINDOW* mainWin;
    WINDOW* dialogWin;
    
    int height, width;
    std::string roomName;
    std::string errorMessage;
    bool isActive;
    
    static constexpr int DIALOG_WIDTH = 60;
    static constexpr int DIALOG_HEIGHT = 12;
    static constexpr int INPUT_WIDTH = 40;
    
    void drawBorder();
    void drawTitle();
    void drawInputField();
    void drawButtons();
    void drawError();
    void handleInput(int ch);
    
public:
    CreateRoomDialog();
    ~CreateRoomDialog();
    
    void draw();
    int show();  // Returns: 1=Create, -1=Cancel
    
    std::string getRoomName() const { return roomName; }
    void setError(const std::string& msg);
    void reset();
};
