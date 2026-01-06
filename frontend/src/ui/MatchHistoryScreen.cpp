#include "ui/MatchHistoryScreen.h"
#include <sstream>
#include <iomanip>

MatchHistoryScreen::MatchHistoryScreen()
    : selectedIndex(0),
      scrollOffset(0),
      showDetail(false) {
    
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

MatchHistoryScreen::~MatchHistoryScreen() {
    if (mainWin) delwin(mainWin);
}

void MatchHistoryScreen::drawBorder() {
    box(mainWin, 0, 0);
}

void MatchHistoryScreen::drawTitle() {
    wattron(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    int startY = 2;
    int startX = (width - 50) / 2;
    mvwprintw(mainWin, startY++, startX, " __  __    _  _____ ____ _   _   _   _ ___ ____ _____ ");
    mvwprintw(mainWin, startY++, startX, "|  \\/  |  / \\|_   _/ ___| | | | | | | |_ _/ ___|_   _|");
    mvwprintw(mainWin, startY++, startX, "| |\\/| | / _ \\ | || |   | |_| | | |_| || |\\___ \\ | |  ");
    mvwprintw(mainWin, startY++, startX, "| |  | |/ ___ \\| || |___|  _  | |  _  || | ___) || |  ");
    mvwprintw(mainWin, startY++, startX, "|_|  |_/_/   \\_\\_| \\____|_| |_| |_| |_|___|____/ |_|  ");
    
    wattroff(mainWin, COLOR_PAIR(1) | A_BOLD);
}

void MatchHistoryScreen::drawHeader() {
    int y = LIST_START_Y - 1;
    
    wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(mainWin, y, 5, "%-8s %-20s %-10s %-20s %s",
              "ID", "OPPONENT", "RESULT", "DATE", "SUMMARY");
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    
    // Divider
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    for (int x = 3; x < width - 3; x++) {
        mvwaddch(mainWin, y + 1, x, '-');
    }
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
}

void MatchHistoryScreen::drawMatchList() {
    if (matches.empty()) {
        drawNoData();
        return;
    }
    
    int displayCount = (matches.size() < ITEMS_PER_PAGE) ? matches.size() : ITEMS_PER_PAGE;
    int startY = LIST_START_Y + 1;
    
    for (int i = 0; i < displayCount && (scrollOffset + i) < matches.size(); i++) {
        int idx = scrollOffset + i;
        const MatchRecord& match = matches[idx];
        
        bool isSelected = (idx == selectedIndex);
        
        if (isSelected) {
            wattron(mainWin, COLOR_PAIR(2) | A_REVERSE | A_BOLD);
        }
        
        // Format the row
        std::string resultStr = getResultString(match.result_code);
        std::string dateStr = formatTimestamp(match.timestamp);
        
        // Truncate summary if too long
        std::string summary = match.summary;
        if (summary.length() > 25) {
            summary = summary.substr(0, 22) + "...";
        }
        
        mvwprintw(mainWin, startY + i, 5, "%-8u %-20s", 
                  match.match_id, match.opponent.c_str());
        
        if (!isSelected) {
            wattroff(mainWin, COLOR_PAIR(2) | A_REVERSE | A_BOLD);
            wattron(mainWin, COLOR_PAIR(getResultColor(match.result_code)) | A_BOLD);
        }
        
        wprintw(mainWin, " %-10s", resultStr.c_str());
        
        if (!isSelected) {
            wattroff(mainWin, COLOR_PAIR(getResultColor(match.result_code)) | A_BOLD);
            wattron(mainWin, COLOR_PAIR(3));
        }
        
        wprintw(mainWin, " %-20s %s", dateStr.c_str(), summary.c_str());
        
        if (isSelected) {
            wattroff(mainWin, COLOR_PAIR(2) | A_REVERSE | A_BOLD);
        } else {
            wattroff(mainWin, COLOR_PAIR(3));
        }
    }
}

void MatchHistoryScreen::drawDetailView() {
    if (matches.empty() || selectedIndex >= matches.size()) return;
    
    const MatchRecord& match = matches[selectedIndex];
    
    int detailY = LIST_START_Y + LIST_HEIGHT + 2;
    
    // Detail box
    wattron(mainWin, COLOR_PAIR(6) | A_BOLD);
    mvwprintw(mainWin, detailY, (width - 30) / 2, "=== MATCH DETAILS ===");
    wattroff(mainWin, COLOR_PAIR(6) | A_BOLD);
    
    detailY += 2;
    
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, detailY++, 5, "Match ID:  %u", match.match_id);
    mvwprintw(mainWin, detailY++, 5, "Opponent:  %s", match.opponent.c_str());
    mvwprintw(mainWin, detailY++, 5, "Result:    ");
    wattroff(mainWin, COLOR_PAIR(3));
    
    wattron(mainWin, COLOR_PAIR(getResultColor(match.result_code)) | A_BOLD);
    wprintw(mainWin, "%s", getResultString(match.result_code).c_str());
    wattroff(mainWin, COLOR_PAIR(getResultColor(match.result_code)) | A_BOLD);
    
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, detailY++, 5, "Date:      %s", formatTimestamp(match.timestamp).c_str());
    mvwprintw(mainWin, detailY++, 5, "Summary:   %s", match.summary.c_str());
    wattroff(mainWin, COLOR_PAIR(3));
}

void MatchHistoryScreen::drawNoData() {
    wattron(mainWin, COLOR_PAIR(5));
    mvwprintw(mainWin, LIST_START_Y + 5, (width - 30) / 2, "No match history available");
    mvwprintw(mainWin, LIST_START_Y + 7, (width - 35) / 2, "Play some games to see your history!");
    wattroff(mainWin, COLOR_PAIR(5));
}

void MatchHistoryScreen::drawInstructions() {
    int y = height - 3;
    
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    
    if (!matches.empty()) {
        mvwprintw(mainWin, y, 5, "[↑↓] Navigate  [Enter] Toggle Details  [ESC/B] Back to Menu");
    } else {
        mvwprintw(mainWin, y, 5, "[ESC/B] Back to Menu");
    }
    
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
    
    // Show scroll indicator
    if (!matches.empty() && matches.size() > ITEMS_PER_PAGE) {
        wattron(mainWin, COLOR_PAIR(5));
        mvwprintw(mainWin, y, width - 30, "Showing %d-%ld of %lu matches",
                  scrollOffset + 1,
                  (scrollOffset + ITEMS_PER_PAGE < matches.size()) ? 
                      scrollOffset + ITEMS_PER_PAGE : matches.size(),
                  matches.size());
        wattroff(mainWin, COLOR_PAIR(5));
    }
}

void MatchHistoryScreen::draw() {
    werase(mainWin);
    drawBorder();
    drawTitle();
    
    if (!matches.empty()) {
        drawHeader();
        drawMatchList();
        
        if (showDetail) {
            drawDetailView();
        }
    } else {
        drawNoData();
    }
    
    drawInstructions();
    wrefresh(mainWin);
}

void MatchHistoryScreen::handleNavigation(int ch) {
    if (matches.empty()) return;
    
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
            if (selectedIndex < matches.size() - 1) {
                selectedIndex++;
                if (selectedIndex >= scrollOffset + ITEMS_PER_PAGE) {
                    scrollOffset++;
                }
            }
            break;
            
        case 10:  // Enter
        case KEY_ENTER:
            showDetail = !showDetail;
            break;
    }
}

int MatchHistoryScreen::handleInput() {
    int ch = wgetch(mainWin);
    
    if (ch == 27 || ch == 'b' || ch == 'B') {  // ESC or B
        return -1;  // Back to menu
    }
    
    if (ch == KEY_UP || ch == KEY_DOWN || ch == 10 || ch == KEY_ENTER) {
        handleNavigation(ch);
    }
    
    return 0;  // Continue
}

std::string MatchHistoryScreen::formatTimestamp(uint32_t timestamp) const {
    time_t time = static_cast<time_t>(timestamp);
    struct tm* timeinfo = localtime(&time);
    
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeinfo);
    return std::string(buffer);
}

std::string MatchHistoryScreen::getResultString(uint8_t result_code) const {
    switch(result_code) {
        case 0: return "LOSS";
        case 1: return "WIN";
        case 2: return "DRAW";
        default: return "UNKNOWN";
    }
}

int MatchHistoryScreen::getResultColor(uint8_t result_code) const {
    switch(result_code) {
        case 0: return 4;  // Red for loss
        case 1: return 2;  // Green for win
        case 2: return 5;  // Yellow for draw
        default: return 3; // White for unknown
    }
}

void MatchHistoryScreen::setMatches(const std::vector<MatchRecord>& matchList) {
    matches = matchList;
    selectedIndex = 0;
    scrollOffset = 0;
    showDetail = false;
}

void MatchHistoryScreen::reset() {
    matches.clear();
    selectedIndex = 0;
    scrollOffset = 0;
    showDetail = false;
}