#include "ui/RoomLobbyScreen.h"

RoomLobbyScreen::RoomLobbyScreen()
    : currentUser(""),
      selectedButton(RoomButton::READY),
      roomId(0) {
    
    getmaxyx(stdscr, height, width);
    mainWin = newwin(height, width, 0, 0);
    keypad(mainWin, TRUE);
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
}

RoomLobbyScreen::~RoomLobbyScreen() {
    if (mainWin) delwin(mainWin);
}

void RoomLobbyScreen::drawBorder() {
    box(mainWin, 0, 0);
}

void RoomLobbyScreen::drawTitle() {
    wattron(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    int startY = 2;
    int startX = (width - 45) / 2;
    mvwprintw(mainWin, startY++, startX, "  ____      _    __  __ _____   _     ___  ____  ______   __");
    mvwprintw(mainWin, startY++, startX, " / ___|    / \\  |  \\/  | ____| | |   / _ \\| __ )|  _ \\ \\ / /");
    mvwprintw(mainWin, startY++, startX, "| |  _    / _ \\ | |\\/| |  _|   | |  | | | |  _ \\| |_) \\ V / ");
    mvwprintw(mainWin, startY++, startX, "| |_| |  / ___ \\| |  | | |___  | |__| |_| | |_) |  _ < | |  ");
    mvwprintw(mainWin, startY++, startX, " \\____|_/_/   \\_\\_|  |_|_____| |_____\\___/|____/|_| \\_\\|_|  ");
    
    wattroff(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    // Room ID
    wattron(mainWin, COLOR_PAIR(5));
    mvwprintw(mainWin, startY + 1, (width - 20) / 2, "Room ID: #%u", roomId);
    wattroff(mainWin, COLOR_PAIR(5));
}

void RoomLobbyScreen::drawPlayerBox(const PlayerInfo& player, int x, int y) {
    bool isCurrentUser = (player.username == currentUser);
    int borderColor = isCurrentUser ? 5 : 3;
    
    // Draw box
    wattron(mainWin, COLOR_PAIR(borderColor));
    for (int i = 0; i < PLAYER_BOX_HEIGHT; i++) {
        mvwaddch(mainWin, y + i, x, '|');
        mvwaddch(mainWin, y + i, x + PLAYER_BOX_WIDTH - 1, '|');
    }
    for (int i = 0; i < PLAYER_BOX_WIDTH; i++) {
        mvwaddch(mainWin, y, x + i, '-');
        mvwaddch(mainWin, y + PLAYER_BOX_HEIGHT - 1, x + i, '-');
    }
    mvwaddch(mainWin, y, x, '+');
    mvwaddch(mainWin, y, x + PLAYER_BOX_WIDTH - 1, '+');
    mvwaddch(mainWin, y + PLAYER_BOX_HEIGHT - 1, x, '+');
    mvwaddch(mainWin, y + PLAYER_BOX_HEIGHT - 1, x + PLAYER_BOX_WIDTH - 1, '+');
    wattroff(mainWin, COLOR_PAIR(borderColor));
    
    // Host badge
    if (player.isHost) {
        wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
        mvwprintw(mainWin, y + 1, x + 2, "[HOST]");
        wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    }
    
    // Username
    wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(mainWin, y + 3, x + 2, "%s", player.username.c_str());
    wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
    
    if (isCurrentUser) {
        wattron(mainWin, COLOR_PAIR(5));
        mvwprintw(mainWin, y + 3, x + 2 + player.username.length() + 1, "(You)");
        wattroff(mainWin, COLOR_PAIR(5));
    }
    
    // Divider
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    for (int i = 2; i < PLAYER_BOX_WIDTH - 2; i++) {
        mvwaddch(mainWin, y + 4, x + i, '-');
    }
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
    
    // Stats
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, y + 5, x + 2, "Wins:   %d", player.wins);
    mvwprintw(mainWin, y + 6, x + 2, "Losses: %d", player.losses);
    mvwprintw(mainWin, y + 7, x + 2, "Draws:  %d", player.draws);
    wattroff(mainWin, COLOR_PAIR(3));
    
    // Ready status
    if (player.isReady) {
        wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(mainWin, y + 9, x + (PLAYER_BOX_WIDTH - 7) / 2, "[READY]");
        wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(4));
        mvwprintw(mainWin, y + 9, x + (PLAYER_BOX_WIDTH - 12) / 2, "[NOT READY]");
        wattroff(mainWin, COLOR_PAIR(4));
    }
}

void RoomLobbyScreen::drawVersusSymbol() {
    int y = 12;
    int x = width / 2 - 3;
    
    wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(mainWin, y, x, "__   _____ ");
    mvwprintw(mainWin, y + 1, x, "\\ \\ / / __|");
    mvwprintw(mainWin, y + 2, x, " \\ V /\\__ \\");
    mvwprintw(mainWin, y + 3, x, "  \\_/ |___/");
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
}

void RoomLobbyScreen::drawButtons() {
    int buttonY = height - 8;
    int centerX = width / 2;
    
    // Ready button
    if (selectedButton == RoomButton::READY) {
        wattron(mainWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
        mvwprintw(mainWin, buttonY, centerX - 30, " >> [  READY  ] << ");
        wattroff(mainWin, COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    } else {
        wattron(mainWin, COLOR_PAIR(2));
        mvwprintw(mainWin, buttonY, centerX - 30, "    [  READY  ]    ");
        wattroff(mainWin, COLOR_PAIR(2));
    }
    
    // Leave button
    if (selectedButton == RoomButton::LEAVE) {
        wattron(mainWin, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
        mvwprintw(mainWin, buttonY, centerX - 8, " >> [  LEAVE  ] << ");
        wattroff(mainWin, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
    } else {
        wattron(mainWin, COLOR_PAIR(4));
        mvwprintw(mainWin, buttonY, centerX - 8, "    [  LEAVE  ]    ");
        wattroff(mainWin, COLOR_PAIR(4));
    }
    
    // Kick button (only for host)
    if (isHost()) {
        if (selectedButton == RoomButton::KICK) {
            wattron(mainWin, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
            mvwprintw(mainWin, buttonY, centerX + 14, " >> [  KICK   ] << ");
            wattroff(mainWin, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
        } else {
            wattron(mainWin, COLOR_PAIR(6));
            mvwprintw(mainWin, buttonY, centerX + 14, "    [  KICK   ]    ");
            wattroff(mainWin, COLOR_PAIR(6));
        }
    }
}

void RoomLobbyScreen::drawInstructions() {
    int y = height - 3;
    
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    if (isHost()) {
        mvwprintw(mainWin, y, 10, "[←→] Switch  [Enter] Select  [ESC] Cancel");
    } else {
        mvwprintw(mainWin, y, 10, "[←→] Switch between Ready/Leave  [Enter] Select  [ESC] Cancel");
    }
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
    
    // Both ready message
    if (player1.isReady && player2.isReady) {
        wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(mainWin, y + 1, (width - 35) / 2, "Both players ready! Starting soon...");
        wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
    }
}

void RoomLobbyScreen::draw() {
    werase(mainWin);
    drawBorder();
    drawTitle();
    
    // Draw player boxes
    int boxY = 10;
    int player1X = (width / 2) - PLAYER_BOX_WIDTH - 8;
    int player2X = (width / 2) + 8;
    
    drawPlayerBox(player1, player1X, boxY);
    drawPlayerBox(player2, player2X, boxY);
    drawVersusSymbol();
    
    drawButtons();
    drawInstructions();
    wrefresh(mainWin);
}

int RoomLobbyScreen::handleInput() {
    int ch = wgetch(mainWin);
    
    switch(ch) {
        case KEY_LEFT:
            if (isHost()) {
                if (selectedButton == RoomButton::LEAVE) {
                    selectedButton = RoomButton::READY;
                } else if (selectedButton == RoomButton::KICK) {
                    selectedButton = RoomButton::LEAVE;
                }
            } else {
                if (selectedButton == RoomButton::LEAVE) {
                    selectedButton = RoomButton::READY;
                }
            }
            break;
            
        case KEY_RIGHT:
            if (isHost()) {
                if (selectedButton == RoomButton::READY) {
                    selectedButton = RoomButton::LEAVE;
                } else if (selectedButton == RoomButton::LEAVE) {
                    selectedButton = RoomButton::KICK;
                }
            } else {
                if (selectedButton == RoomButton::READY) {
                    selectedButton = RoomButton::LEAVE;
                }
            }
            break;
            
        case 10:  // Enter
        case KEY_ENTER:
            if (selectedButton == RoomButton::READY) {
                return 1;  // Toggle ready
            } else if (selectedButton == RoomButton::LEAVE) {
                return 2;  // Leave room
            } else if (selectedButton == RoomButton::KICK && isHost()) {
                return 3;  // Kick player
            }
            break;
            
        case 27:  // ESC
            return 2;  // Leave room
            break;
    }
    
    // Check if both ready -> start game
    if (player1.isReady && player2.isReady) {
        return 4;  // Start game
    }
    
    return 0;  // Continue
}

void RoomLobbyScreen::setPlayers(const PlayerInfo& p1, const PlayerInfo& p2) {
    player1 = p1;
    player2 = p2;
    player1.isHost = true;
}

void RoomLobbyScreen::setCurrentUser(const std::string& username) {
    currentUser = username;
    
    // Set default button based on role
    if (isHost()) {
        selectedButton = RoomButton::READY;
    } else {
        selectedButton = RoomButton::READY;
    }
}

void RoomLobbyScreen::setPlayerReady(const std::string& username, bool ready) {
    if (player1.username == username) {
        player1.isReady = ready;
    } else if (player2.username == username) {
        player2.isReady = ready;
    }
}

void RoomLobbyScreen::reset() {
    player1 = PlayerInfo();
    player2 = PlayerInfo();
    selectedButton = RoomButton::READY;
    roomId = 0;
}