#ifndef ROOM_LOBBY_SCREEN_H
#define ROOM_LOBBY_SCREEN_H

#include <ncurses.h>
#include <string>

struct PlayerInfo {
    std::string username;
    int wins;
    int losses;
    int draws;
    bool isReady;
    bool isHost;
    
    PlayerInfo() : username(""), wins(0), losses(0), draws(0), 
                   isReady(false), isHost(false) {}
    
    PlayerInfo(const std::string& name, int w, int l, int d, bool host = false)
        : username(name), wins(w), losses(l), draws(d), 
          isReady(false), isHost(host) {}
};

enum class RoomButton {
    READY,
    LEAVE,
    KICK  // Only for host
};

class RoomLobbyScreen {
private:
    WINDOW* mainWin;
    int height, width;
    
    // State
    PlayerInfo player1;  // Host
    PlayerInfo player2;  // Guest
    std::string currentUser;
    RoomButton selectedButton;
    uint32_t roomId;
    
    // UI Constants
    static const int PLAYER_BOX_WIDTH = 35;
    static const int PLAYER_BOX_HEIGHT = 12;
    
    // Private methods
    void drawBorder();
    void drawTitle();
    void drawPlayerBox(const PlayerInfo& player, int x, int y);
    void drawVersusSymbol();
    void drawButtons();
    void drawInstructions();
    
    bool isHost() const { return currentUser == player1.username; }
    
public:
    RoomLobbyScreen();
    ~RoomLobbyScreen();
    
    void draw();
    int handleInput();  // Returns: 1=ready, 2=leave, 3=kick, 4=start game
    
    void setRoomId(uint32_t id) { roomId = id; }
    void setPlayers(const PlayerInfo& p1, const PlayerInfo& p2);
    void setCurrentUser(const std::string& username);
    void setPlayerReady(const std::string& username, bool ready);
    std::string getCurrentUser() const { return currentUser; }
    void reset();
};

#endif // ROOM_LOBBY_SCREEN_H