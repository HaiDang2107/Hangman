#include "ui/RankingsScreen.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

RankingsScreen::RankingsScreen()
    : selectedIndex(0),
      scrollOffset(0),
      currentUser("") {
    
    getmaxyx(stdscr, height, width);
    
    mainWin = newwin(height, width, 0, 0);
    keypad(mainWin, TRUE);
    
    // Colors
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
}

RankingsScreen::~RankingsScreen() {
    if (mainWin) delwin(mainWin);
}

void RankingsScreen::drawBorder() {
    box(mainWin, 0, 0);
}

void RankingsScreen::drawTitle() {
    wattron(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    int startY = 2;
    int startX = (width - 50) / 2;
    mvwprintw(mainWin, startY++, startX, " ____      _    _   _ _  _____ _   _  ____ ____  ");
    mvwprintw(mainWin, startY++, startX, "|  _ \\    / \\  | \\ | | |/ /_ _| \\ | |/ ___/ ___| ");
    mvwprintw(mainWin, startY++, startX, "| |_) |  / _ \\ |  \\| | ' / | ||  \\| | |  _\\___ \\ ");
    mvwprintw(mainWin, startY++, startX, "|  _ <  / ___ \\| |\\  | . \\ | || |\\  | |_| |___) |");
    mvwprintw(mainWin, startY++, startX, "|_| \\_\\/_/   \\_\\_| \\_|_|\\_\\___|_| \\_|\\____|____/ ");
    
    wattroff(mainWin, COLOR_PAIR(1) | A_BOLD);
}

void RankingsScreen::drawPodium() {
    if (rankings.size() < 3) return;
    
    int podiumY = 7;
    int centerX = width / 2;
    
    // 2nd place (left)
    if (rankings.size() >= 2) {
        wattron(mainWin, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(mainWin, podiumY, centerX - 25, "[2nd]");
        mvwprintw(mainWin, podiumY + 1, centerX - 28, "%s", rankings[1].username.c_str());
        wattroff(mainWin, COLOR_PAIR(3) | A_BOLD);
        wattron(mainWin, COLOR_PAIR(2));
        mvwprintw(mainWin, podiumY + 2, centerX - 28, "%u wins", rankings[1].wins);
        wattroff(mainWin, COLOR_PAIR(2));
    }
    
    // 1st place (center)
    wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(mainWin, podiumY - 1, centerX - 5, "[1st]");
    mvwprintw(mainWin, podiumY, centerX - 8, "%s", rankings[0].username.c_str());
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(mainWin, podiumY + 1, centerX - 8, "%u wins", rankings[0].wins);
    wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
    
    // 3rd place (right)
    if (rankings.size() >= 3) {
        wattron(mainWin, COLOR_PAIR(6) | A_BOLD);
        mvwprintw(mainWin, podiumY, centerX + 20, "[3rd]");
        mvwprintw(mainWin, podiumY + 1, centerX + 17, "%s", rankings[2].username.c_str());
        wattroff(mainWin, COLOR_PAIR(6) | A_BOLD);
        wattron(mainWin, COLOR_PAIR(2));
        mvwprintw(mainWin, podiumY + 2, centerX + 17, "%u wins", rankings[2].wins);
        wattroff(mainWin, COLOR_PAIR(2));
    }
}

void RankingsScreen::drawHeader() {
    int y = LIST_START_Y - 1;
    
    wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(mainWin, y, 5, "%-5s %-20s %-8s %-8s %-8s %-10s %s",
              "RANK", "PLAYER", "WINS", "LOSSES", "DRAWS", "TOTAL", "WIN RATE");
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    
    // Divider
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    for (int x = 3; x < width - 3; x++) {
        mvwaddch(mainWin, y + 1, x, '-');
    }
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
}

void RankingsScreen::drawRankingsList() {
    if (rankings.empty()) {
        drawNoData();
        return;
    }
    
    int displayCount = (rankings.size() < ITEMS_PER_PAGE) ? rankings.size() : ITEMS_PER_PAGE;
    int startY = LIST_START_Y + 1;
    
    for (int i = 0; i < displayCount && (scrollOffset + i) < rankings.size(); i++) {
        int idx = scrollOffset + i;
        const PlayerRanking& player = rankings[idx];
        
        bool isSelected = (idx == selectedIndex);
        bool isCurrentUser = (player.username == currentUser);
        
        int colorPair = 3;  // Default white
        int attrs = 0;
        
        if (isCurrentUser) {
            colorPair = 5;  // Yellow for current user
            attrs = A_BOLD;
        }
        
        if (isSelected) {
            attrs |= A_REVERSE;
        }
        
        // Medal for top 3
        std::string medal = "";
        if (idx == 0) {
            medal = "[1st]";
            if (!isSelected && !isCurrentUser) colorPair = 5;  // Gold
        } else if (idx == 1) {
            medal = "[2nd]";
            if (!isSelected && !isCurrentUser) colorPair = 3;  // Silver
        } else if (idx == 2) {
            medal = "[3rd]";
            if (!isSelected && !isCurrentUser) colorPair = 6;  // Bronze
        }
        
        wattron(mainWin, COLOR_PAIR(colorPair) | attrs);
        
        // Rank
        if (!medal.empty()) {
            mvwprintw(mainWin, startY + i, 5, "%-5s", medal.c_str());
        } else {
            mvwprintw(mainWin, startY + i, 5, "#%-4d", idx + 1);
        }
        
        // Player name
        wprintw(mainWin, " %-20s", player.username.c_str());
        
        // Stats
        wprintw(mainWin, " %-8u %-8u %-8u %-10u %.1f%%",
                player.wins,
                player.losses,
                player.draws,
                player.getTotalMatches(),
                player.getWinRate());
        
        wattroff(mainWin, COLOR_PAIR(colorPair) | attrs);
    }
}

void RankingsScreen::drawNoData() {
    wattron(mainWin, COLOR_PAIR(5));
    mvwprintw(mainWin, LIST_START_Y + 5, (width - 30) / 2, "No rankings data available");
    mvwprintw(mainWin, LIST_START_Y + 7, (width - 35) / 2, "Be the first to play and rank up!");
    wattroff(mainWin, COLOR_PAIR(5));
}

void RankingsScreen::drawInstructions() {
    int y = height - 3;
    
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    
    if (!rankings.empty()) {
        mvwprintw(mainWin, y, 5, "[↑↓] Navigate  [ESC/B] Back to Menu");
    } else {
        mvwprintw(mainWin, y, 5, "[ESC/B] Back to Menu");
    }
    
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
    
    // Show scroll indicator
    if (!rankings.empty() && rankings.size() > ITEMS_PER_PAGE) {
        wattron(mainWin, COLOR_PAIR(5));
        mvwprintw(mainWin, y, width - 30, "Showing %d-%ld of %lu players",
                  scrollOffset + 1,
                  (scrollOffset + ITEMS_PER_PAGE < rankings.size()) ? 
                      scrollOffset + ITEMS_PER_PAGE : rankings.size(),
                  rankings.size());
        wattroff(mainWin, COLOR_PAIR(5));
    }
    
    // Legend for current user
    if (!currentUser.empty()) {
        wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
        mvwprintw(mainWin, y - 1, width - 30, "* Yellow = You");
        wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    }
}

void RankingsScreen::draw() {
    werase(mainWin);
    drawBorder();
    drawTitle();
    
    if (!rankings.empty()) {
        drawPodium();
        drawHeader();
        drawRankingsList();
    } else {
        drawNoData();
    }
    
    drawInstructions();
    wrefresh(mainWin);
}

void RankingsScreen::handleNavigation(int ch) {
    if (rankings.empty()) return;
    
    switch(ch) {
        case KEY_UP:
            if (selectedIndex > 0) {
                selectedIndex--;
                if (selectedIndex < scrollOffset) {
                    scrollOffset--;
                }
            }
            break;
            
        case KEY_DOWN:
            if (selectedIndex < rankings.size() - 1) {
                selectedIndex++;
                if (selectedIndex >= scrollOffset + ITEMS_PER_PAGE) {
                    scrollOffset++;
                }
            }
            break;
    }
}

int RankingsScreen::handleInput() {
    int ch = wgetch(mainWin);
    
    if (ch == 27 || ch == 'b' || ch == 'B') {  // ESC or B
        return -1;  // Back to menu
    }
    
    if (ch == KEY_UP || ch == KEY_DOWN) {
        handleNavigation(ch);
    }
    
    return 0;  // Continue
}

void RankingsScreen::setRankings(const std::vector<PlayerRanking>& rankList) {
    rankings = rankList;
    
    // Sort by wins (descending)
    std::sort(rankings.begin(), rankings.end(), 
        [](const PlayerRanking& a, const PlayerRanking& b) {
            if (a.wins != b.wins) return a.wins > b.wins;
            if (a.losses != b.losses) return a.losses < b.losses;
            return a.draws > b.draws;
        });
    
    selectedIndex = 0;
    scrollOffset = 0;
    
    // Auto-scroll to current user if exists
    if (!currentUser.empty()) {
        for (size_t i = 0; i < rankings.size(); i++) {
            if (rankings[i].username == currentUser) {
                selectedIndex = i;
                if (i > ITEMS_PER_PAGE / 2) {
                    scrollOffset = i - ITEMS_PER_PAGE / 2;
                }
                break;
            }
        }
    }
}

void RankingsScreen::setCurrentUser(const std::string& username) {
    currentUser = username;
}

void RankingsScreen::reset() {
    rankings.clear();
    selectedIndex = 0;
    scrollOffset = 0;
}