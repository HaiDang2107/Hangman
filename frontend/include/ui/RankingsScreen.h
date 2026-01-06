#ifndef RANKINGS_SCREEN_H
#define RANKINGS_SCREEN_H

#include <ncurses.h>
#include <string>
#include <vector>

struct PlayerRanking {
    std::string username;
    uint32_t wins;
    uint32_t losses;
    uint32_t draws;
    
    PlayerRanking(const std::string& name, uint32_t w, uint32_t l, uint32_t d)
        : username(name), wins(w), losses(l), draws(d) {}
    
    uint32_t getTotalMatches() const { return wins + losses + draws; }
    float getWinRate() const { 
        uint32_t total = getTotalMatches();
        return total > 0 ? (static_cast<float>(wins) / total * 100.0f) : 0.0f;
    }
};

class RankingsScreen {
private:
    WINDOW* mainWin;
    int height, width;
    
    // State
    std::vector<PlayerRanking> rankings;
    int selectedIndex;
    int scrollOffset;
    std::string currentUser;  // To highlight current user
    
    // UI Constants
    static const int LIST_START_Y = 11;
    static const int LIST_HEIGHT = 15;
    static const int ITEMS_PER_PAGE = 12;
    
    // Private methods
    void drawBorder();
    void drawTitle();
    void drawHeader();
    void drawRankingsList();
    void drawInstructions();
    void drawNoData();
    void drawPodium();
    
    void handleNavigation(int ch);
    
public:
    RankingsScreen();
    ~RankingsScreen();
    
    void draw();
    int handleInput();  // Returns: -1=back to menu, 0=continue
    
    void setRankings(const std::vector<PlayerRanking>& rankList);
    void setCurrentUser(const std::string& username);
    void reset();
};

#endif // RANKINGS_SCREEN_H