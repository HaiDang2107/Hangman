#ifndef MATCH_HISTORY_SCREEN_H
#define MATCH_HISTORY_SCREEN_H

#include <ncurses.h>
#include <string>
#include <vector>
#include <ctime>

struct MatchRecord {
    uint32_t match_id;
    std::string opponent;
    uint8_t result_code;  // 0=loss, 1=win, 2=draw
    uint32_t timestamp;
    std::string summary;
    
    MatchRecord(uint32_t id, const std::string& opp, uint8_t result, 
                uint32_t time, const std::string& sum)
        : match_id(id), opponent(opp), result_code(result), 
          timestamp(time), summary(sum) {}
};

class MatchHistoryScreen {
private:
    WINDOW* mainWin;
    int height, width;
    
    // State
    std::vector<MatchRecord> matches;
    int selectedIndex;
    int scrollOffset;
    bool showDetail;
    
    // UI Constants
    static const int LIST_START_Y = 10;
    static const int LIST_HEIGHT = 15;
    static const int ITEMS_PER_PAGE = 10;
    
    // Private methods
    void drawBorder();
    void drawTitle();
    void drawHeader();
    void drawMatchList();
    void drawDetailView();
    void drawInstructions();
    void drawNoData();
    
    void handleNavigation(int ch);
    std::string formatTimestamp(uint32_t timestamp) const;
    std::string getResultString(uint8_t result_code) const;
    int getResultColor(uint8_t result_code) const;
    
public:
    MatchHistoryScreen();
    ~MatchHistoryScreen();
    
    void draw();
    int handleInput();  // Returns: -1=back to menu, 0=continue
    
    void setMatches(const std::vector<MatchRecord>& matchList);
    void reset();
};

#endif // MATCH_HISTORY_SCREEN_H