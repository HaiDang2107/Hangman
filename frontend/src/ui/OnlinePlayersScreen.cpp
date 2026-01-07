#include "ui/OnlinePlayersScreen.h"

OnlinePlayersScreen::OnlinePlayersScreen()
    : selectedIndex(0),
      scrollOffset(0),
      currentUser(""),
      inviteStatus(InviteStatus::NONE),
      invitedPlayer(""),
      inviteCountdown(0) {
    
    getmaxyx(stdscr, height, width);
    mainWin = newwin(height, width, 0, 0);
    keypad(mainWin, TRUE);
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
}

OnlinePlayersScreen::~OnlinePlayersScreen() {
    if (mainWin) delwin(mainWin);
}

void OnlinePlayersScreen::drawBorder() {
    box(mainWin, 0, 0);
}

void OnlinePlayersScreen::drawTitle() {
    wattron(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    int startY = 2;
    int startX = (width - 50) / 2;
    mvwprintw(mainWin, startY++, startX, "  ___  _   _ _     ___ _   _ _____   ____  _        _ __   _______ ____  ____  ");
    mvwprintw(mainWin, startY++, startX, " / _ \\| \\ | | |   |_ _| \\ | | ____| |  _ \\| |      / \\ \\ / / ____|  _ \\/ ___| ");
    mvwprintw(mainWin, startY++, startX, "| | | |  \\| | |    | ||  \\| |  _|   | |_) | |     / _ \\ V /|  _| | |_) \\___ \\ ");
    mvwprintw(mainWin, startY++, startX, "| |_| | |\\  | |___ | || |\\  | |___  |  __/| |___ / ___ \\| | | |___|  _ < ___) |");
    mvwprintw(mainWin, startY++, startX, " \\___/|_| \\_|_____|___|_| \\_|_____| |_|   |_____/_/   \\_\\_| |_____|_| \\_\\____/ ");
    
    wattroff(mainWin, COLOR_PAIR(1) | A_BOLD);
}

void OnlinePlayersScreen::drawHeader() {
    int y = LIST_START_Y - 1;
    
    wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(mainWin, y, 10, "%-30s %s", "PLAYER", "STATUS");
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    for (int x = 8; x < width - 8; x++) {
        mvwaddch(mainWin, y + 1, x, '-');
    }
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
}

void OnlinePlayersScreen::drawPlayerList() {
    if (players.empty()) {
        drawNoData();
        return;
    }
    
    int displayCount = (players.size() < ITEMS_PER_PAGE) ? players.size() : ITEMS_PER_PAGE;
    int startY = LIST_START_Y + 1;
    
    for (int i = 0; i < displayCount && (scrollOffset + i) < players.size(); i++) {
        int idx = scrollOffset + i;
        const OnlinePlayer& player = players[idx];
        
        bool isSelected = (idx == selectedIndex);
        bool isSelf = (player.username == currentUser);
        
        if (isSelf) continue;  // Skip self
        
        int colorPair = player.isAvailable ? 2 : 3;
        
        if (isSelected) {
            wattron(mainWin, COLOR_PAIR(colorPair) | A_REVERSE | A_BOLD);
        } else {
            wattron(mainWin, COLOR_PAIR(colorPair));
        }
        
        mvwprintw(mainWin, startY + i, 10, "%-30s", player.username.c_str());
        
        std::string status = player.isAvailable ? "[Available]" : "[In Game]";
        wprintw(mainWin, " %s", status.c_str());
        
        if (isSelected) {
            wattroff(mainWin, COLOR_PAIR(colorPair) | A_REVERSE | A_BOLD);
        } else {
            wattroff(mainWin, COLOR_PAIR(colorPair));
        }
    }
}

void OnlinePlayersScreen::drawNoData() {
    wattron(mainWin, COLOR_PAIR(5));
    mvwprintw(mainWin, LIST_START_Y + 5, (width - 35) / 2, "No other players online right now");
    mvwprintw(mainWin, LIST_START_Y + 7, (width - 30) / 2, "Check back later!");
    wattroff(mainWin, COLOR_PAIR(5));
}

void OnlinePlayersScreen::drawInviteWaiting() {
    if (inviteStatus != InviteStatus::WAITING) return;
    
    int boxY = height / 2 - 4;
    int boxX = (width - 50) / 2;
    int boxWidth = 50;
    int boxHeight = 8;
    
    // Draw box
    wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    for (int y = boxY; y < boxY + boxHeight; y++) {
        for (int x = boxX; x < boxX + boxWidth; x++) {
            if (y == boxY || y == boxY + boxHeight - 1 ||
                x == boxX || x == boxX + boxWidth - 1) {
                mvwaddch(mainWin, y, x, '#');
            } else {
                mvwaddch(mainWin, y, x, ' ');
            }
        }
    }
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    
    // Title
    wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(mainWin, boxY + 1, boxX + 2, "WAITING FOR RESPONSE");
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    
    // Message
    wattron(mainWin, COLOR_PAIR(3));
    std::string msg = "Invitation sent to: " + invitedPlayer;
    mvwprintw(mainWin, boxY + 3, boxX + (boxWidth - msg.length()) / 2, "%s", msg.c_str());
    wattroff(mainWin, COLOR_PAIR(3));
    
    // Countdown
    wattron(mainWin, COLOR_PAIR(inviteCountdown <= 5 ? 4 : 2) | A_BOLD);
    mvwprintw(mainWin, boxY + 5, boxX + (boxWidth - 20) / 2, "Time remaining: %2d seconds", inviteCountdown);
    wattroff(mainWin, COLOR_PAIR(inviteCountdown <= 5 ? 4 : 2) | A_BOLD);
}

void OnlinePlayersScreen::drawInstructions() {
    int y = height - 3;
    
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    
    if (inviteStatus == InviteStatus::WAITING) {
        mvwprintw(mainWin, y, 10, "[ESC] Cancel Invitation");
    } else if (!players.empty()) {
        mvwprintw(mainWin, y, 10, "[↑↓] Navigate  [Enter] Send Challenge  [ESC/B] Back");
    } else {
        mvwprintw(mainWin, y, 10, "[ESC/B] Back to Menu");
    }
    
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
}

void OnlinePlayersScreen::draw() {
    werase(mainWin);
    drawBorder();
    drawTitle();
    
    if (!players.empty()) {
        drawHeader();
        drawPlayerList();
    } else {
        drawNoData();
    }
    
    drawInviteWaiting();
    drawInstructions();
    wrefresh(mainWin);
}

void OnlinePlayersScreen::handleNavigation(int ch) {
    if (players.empty()) return;
    
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
            if (selectedIndex < players.size() - 1) {
                selectedIndex++;
                if (selectedIndex >= scrollOffset + ITEMS_PER_PAGE) {
                    scrollOffset++;
                }
            }
            break;
    }
}

int OnlinePlayersScreen::handleInput() {
    int ch = wgetch(mainWin);
    
    if (ch == 27 || ch == 'b' || ch == 'B') {  // ESC or B
        if (inviteStatus == InviteStatus::WAITING) {
            inviteStatus = InviteStatus::NONE;
            return 0;
        }
        return -1;  // Back to menu
    }
    
    if (inviteStatus == InviteStatus::WAITING) {
        return 0;  // Block input while waiting
    }
    
    if (ch == KEY_UP || ch == KEY_DOWN) {
        handleNavigation(ch);
    }
    
    if (ch == 10 || ch == KEY_ENTER) {
        if (!players.empty() && selectedIndex < players.size()) {
            const OnlinePlayer& selected = players[selectedIndex];
            if (selected.username != currentUser && selected.isAvailable) {
                return 1;  // Send invite
            }
        }
    }
    
    return 0;
}

void OnlinePlayersScreen::setPlayers(const std::vector<OnlinePlayer>& playerList) {
    players = playerList;
    selectedIndex = 0;
    scrollOffset = 0;
}

void OnlinePlayersScreen::setCurrentUser(const std::string& username) {
    currentUser = username;
}

std::string OnlinePlayersScreen::getSelectedPlayer() const {
    if (selectedIndex < players.size()) {
        return players[selectedIndex].username;
    }
    return "";
}

void OnlinePlayersScreen::setInviteStatus(InviteStatus status) {
    inviteStatus = status;
    if (status == InviteStatus::WAITING) {
        invitedPlayer = getSelectedPlayer();
        inviteCountdown = 15;
    }
}

void OnlinePlayersScreen::setInviteCountdown(int seconds) {
    inviteCountdown = seconds;
}

void OnlinePlayersScreen::reset() {
    players.clear();
    selectedIndex = 0;
    scrollOffset = 0;
    inviteStatus = InviteStatus::NONE;
    invitedPlayer = "";
    inviteCountdown = 0;
}