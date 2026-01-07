#ifndef ONLINE_PLAYERS_SCREEN_H
#define ONLINE_PLAYERS_SCREEN_H

#include <ncurses.h>
#include <string>
#include <vector>

struct OnlinePlayer {
    std::string username;
    bool isAvailable;  // Not in game
    
    OnlinePlayer(const std::string& name, bool available = true)
        : username(name), isAvailable(available) {}
};

enum class InviteStatus {
    NONE,
    WAITING,     // Waiting for response (15s)
    ACCEPTED,
    DECLINED,
    TIMEOUT
};

class OnlinePlayersScreen {
private:
    WINDOW* mainWin;
    int height, width;
    
    // State
    std::vector<OnlinePlayer> players;
    int selectedIndex;
    int scrollOffset;
    std::string currentUser;
    
    // Invite state
    InviteStatus inviteStatus;
    std::string invitedPlayer;
    int inviteCountdown;  // seconds remaining
    
    // UI Constants
    static const int LIST_START_Y = 10;
    static const int ITEMS_PER_PAGE = 12;
    
    // Private methods
    void drawBorder();
    void drawTitle();
    void drawHeader();
    void drawPlayerList();
    void drawInstructions();
    void drawNoData();
    void drawInviteWaiting();
    
    void handleNavigation(int ch);
    
public:
    OnlinePlayersScreen();
    ~OnlinePlayersScreen();
    
    void draw();
    int handleInput();  // Returns: -1=back, 1=send invite, 2=invite accepted, 3=invite declined
    
    void setPlayers(const std::vector<OnlinePlayer>& playerList);
    void setCurrentUser(const std::string& username);
    std::string getSelectedPlayer() const;
    
    void setInviteStatus(InviteStatus status);
    void setInviteCountdown(int seconds);
    void reset();
};

#endif // ONLINE_PLAYERS_SCREEN_H