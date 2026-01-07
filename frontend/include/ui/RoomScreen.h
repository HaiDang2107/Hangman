#pragma once
#include <ncurses.h>
#include <string>

enum class RoomOption {
    READY_OR_START = 0,  // READY for guest, START for host
    VIEW_FREE_USERS = 1,
    EXIT_ROOM = 2
};

class RoomScreen {
private:
    WINDOW* mainWin;
    WINDOW* player1Win;  // Host
    WINDOW* player2Win;  // Guest
    
    int height, width;
    RoomOption selectedOption;
    
    // Room info
    std::string roomName;
    std::string hostName;
    std::string guestName;
    std::string currentUserName;  // Who is viewing this screen
    bool hostReady;
    bool guestReady;
    bool isHost;  // Current user is host or not
    
    // Colors
    static constexpr int TITLE_COLOR = 1;
    static constexpr int SELECTED_COLOR = 2;
    static constexpr int NORMAL_COLOR = 3;
    static constexpr int HOST_COLOR = 4;
    static constexpr int GUEST_COLOR = 5;
    static constexpr int READY_COLOR = 6;
    
    // Layout constants
    static constexpr int PLAYER_BOX_WIDTH = 28;
    static constexpr int PLAYER_BOX_HEIGHT = 9;
    static constexpr int MENU_START_Y = 16;
    
    void drawBorder();
    void drawTitle();
    void drawPlayerBoxes();
    void drawMenu();
    void drawInstructions();
    void handleNavigation(int ch);
    void initWindows();
    void cleanupWindows();
    
public:
    RoomScreen(const std::string& room, const std::string& host, const std::string& currentUser, bool host_flag = true);
    ~RoomScreen();
    
    void draw();
    void handleResize();
    int handleInput();  // Returns: 1=Ready, 2=ViewUsers, -1=Exit
    
    // Get main window for external control (e.g., timeout)
    WINDOW* getMainWindow() { return mainWin; }
    
    // Update functions
    void setGuestPlayer(const std::string& name);
    void removeGuestPlayer();
    void setHostReady(bool ready);
    void setGuestReady(bool ready);
    void setRoomName(const std::string& name);
};
