#include "ui/RoomScreen.h"
#include <cstring>
#include <iostream>

void RoomScreen::initWindows() {
    // Get screen size
    getmaxyx(stdscr, height, width);

    // Create main window - leave space for player boxes at top
    int mainWinStartY = 13;  // Player boxes are at Y=4, height=9, so start at 13
    mainWin = newwin(height - mainWinStartY, width, mainWinStartY, 0);
    if (!mainWin) {
        std::cerr << "[ERROR RoomScreen] Failed to create mainWin!" << std::endl;
        return;
    }
    keypad(mainWin, TRUE);
    // No timeout by default - screens will be responsive

    // Create player windows - positioned at top
    int playerY = 4;
    int spacing = 3;
    int totalWidth = 2 * PLAYER_BOX_WIDTH + spacing;
    int startX = (width - totalWidth) / 2;

    player1Win = newwin(PLAYER_BOX_HEIGHT, PLAYER_BOX_WIDTH, playerY, startX);
    player2Win = newwin(PLAYER_BOX_HEIGHT, PLAYER_BOX_WIDTH, playerY, startX + PLAYER_BOX_WIDTH + spacing);

    if (!player1Win || !player2Win) {
        std::cerr << "[ERROR RoomScreen] Failed to create player windows!" << std::endl;
    }
}

void RoomScreen::cleanupWindows() {
    if (mainWin) delwin(mainWin);
    if (player1Win) delwin(player1Win);
    if (player2Win) delwin(player2Win);
    mainWin = nullptr;
    player1Win = nullptr;
    player2Win = nullptr;
}

RoomScreen::RoomScreen(const std::string& room, const std::string& host, const std::string& currentUser, bool host_flag)
    : selectedOption(RoomOption::READY_OR_START),
      roomName(room),
      hostName(host),
      guestName(""),
      currentUserName(currentUser),
      hostReady(false),
      guestReady(false),
      isHost(host_flag) {
    
    initWindows();
    
    // Initialize colors
    init_pair(TITLE_COLOR, COLOR_CYAN, COLOR_BLACK);
    init_pair(SELECTED_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(NORMAL_COLOR, COLOR_WHITE, COLOR_BLACK);
    init_pair(HOST_COLOR, COLOR_YELLOW, COLOR_BLACK);
    init_pair(GUEST_COLOR, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(READY_COLOR, COLOR_GREEN, COLOR_BLACK);
}

RoomScreen::~RoomScreen() {
    cleanupWindows();
}

void RoomScreen::drawBorder() {
    box(mainWin, 0, 0);
}

void RoomScreen::drawTitle() {
    // Simple compact title
    wattron(mainWin, COLOR_PAIR(TITLE_COLOR) | A_BOLD);
    std::string title = "=== GAME ROOM ===";
    mvwprintw(mainWin, 1, (width - title.length()) / 2, "%s", title.c_str());
    wattroff(mainWin, COLOR_PAIR(TITLE_COLOR) | A_BOLD);
    
    // Room name
    wattron(mainWin, COLOR_PAIR(HOST_COLOR));
    std::string roomTitle = "Room: " + roomName;
    mvwprintw(mainWin, 2, (width - roomTitle.length()) / 2, "%s", roomTitle.c_str());
    wattroff(mainWin, COLOR_PAIR(HOST_COLOR));
}

void RoomScreen::drawPlayerBoxes() {
    // Check if windows exist
    if (!player1Win || !player2Win) {
        return;
    }
    
    // Determine who to show on left (YOU) and right (OPPONENT)
    std::string leftName, rightName;
    bool leftReady, rightReady;
    bool rightExists = false;
    
    if (isHost) {
        // Host view: YOU (host) on left, OPPONENT (guest) on right
        leftName = currentUserName;
        leftReady = hostReady;
        if (!guestName.empty()) {
            rightName = guestName;
            rightReady = guestReady;
            rightExists = true;
        }
    } else {
        // Guest view: YOU (guest) on left, OPPONENT (host) on right
        leftName = currentUserName;
        leftReady = guestReady;
        rightName = hostName;
        rightReady = hostReady;
        rightExists = true;
    }
    
    // Draw Player 1 (YOU - Current User) on left
    werase(player1Win);
    wbkgd(player1Win, COLOR_PAIR(HOST_COLOR) | A_DIM);
    box(player1Win, 0, 0);
    
    wattron(player1Win, COLOR_PAIR(HOST_COLOR) | A_BOLD | A_REVERSE);
    mvwprintw(player1Win, 0, (PLAYER_BOX_WIDTH - 6) / 2, " YOU ");
    wattroff(player1Win, COLOR_PAIR(HOST_COLOR) | A_BOLD | A_REVERSE);
    
    // Your avatar (compact stick figure)
    wattron(player1Win, COLOR_PAIR(HOST_COLOR) | A_BOLD);
    mvwprintw(player1Win, 2, (PLAYER_BOX_WIDTH - 5) / 2, "  O  ");
    mvwprintw(player1Win, 3, (PLAYER_BOX_WIDTH - 5) / 2, " /|\\\\ ");
    mvwprintw(player1Win, 4, (PLAYER_BOX_WIDTH - 5) / 2, " / \\\\ ");
    wattroff(player1Win, COLOR_PAIR(HOST_COLOR) | A_BOLD);
    
    // Your name
    wattron(player1Win, COLOR_PAIR(NORMAL_COLOR) | A_BOLD);
    int nameX = (PLAYER_BOX_WIDTH - leftName.length()) / 2;
    mvwprintw(player1Win, 6, nameX, "%s", leftName.c_str());
    wattroff(player1Win, COLOR_PAIR(NORMAL_COLOR) | A_BOLD);
    
    // Your status
    if (leftReady) {
        wattron(player1Win, COLOR_PAIR(READY_COLOR) | A_BOLD);
        mvwprintw(player1Win, 7, (PLAYER_BOX_WIDTH - 7) / 2, "*READY*");
        wattroff(player1Win, COLOR_PAIR(READY_COLOR) | A_BOLD);
    } else {
        wattron(player1Win, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
        mvwprintw(player1Win, 7, (PLAYER_BOX_WIDTH - 9) / 2, "Not Ready");
        wattroff(player1Win, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
    }
    
    wnoutrefresh(player1Win);
    
    // Draw Player 2 (OPPONENT) on right
    werase(player2Win);
    wbkgd(player2Win, COLOR_PAIR(GUEST_COLOR) | A_DIM);
    box(player2Win, 0, 0);
    
    wattron(player2Win, COLOR_PAIR(GUEST_COLOR) | A_BOLD | A_REVERSE);
    mvwprintw(player2Win, 0, (PLAYER_BOX_WIDTH - 10) / 2, " OPPONENT ");
    wattroff(player2Win, COLOR_PAIR(GUEST_COLOR) | A_BOLD | A_REVERSE);
    
    if (rightExists) {
        // Opponent avatar (compact stick figure)
        wattron(player2Win, COLOR_PAIR(GUEST_COLOR) | A_BOLD);
        mvwprintw(player2Win, 2, (PLAYER_BOX_WIDTH - 5) / 2, "  O  ");
        mvwprintw(player2Win, 3, (PLAYER_BOX_WIDTH - 5) / 2, " /|\\\\ ");
        mvwprintw(player2Win, 4, (PLAYER_BOX_WIDTH - 5) / 2, " / \\\\ ");
        wattroff(player2Win, COLOR_PAIR(GUEST_COLOR) | A_BOLD);
        
        // Opponent name
        wattron(player2Win, COLOR_PAIR(NORMAL_COLOR) | A_BOLD);
        nameX = (PLAYER_BOX_WIDTH - rightName.length()) / 2;
        mvwprintw(player2Win, 6, nameX, "%s", rightName.c_str());
        wattroff(player2Win, COLOR_PAIR(NORMAL_COLOR) | A_BOLD);
        
        // Opponent status
        if (rightReady) {
            wattron(player2Win, COLOR_PAIR(READY_COLOR) | A_BOLD);
            mvwprintw(player2Win, 7, (PLAYER_BOX_WIDTH - 7) / 2, "*READY*");
            wattroff(player2Win, COLOR_PAIR(READY_COLOR) | A_BOLD);
        } else {
            wattron(player2Win, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
            mvwprintw(player2Win, 7, (PLAYER_BOX_WIDTH - 9) / 2, "Not Ready");
            wattroff(player2Win, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
        }
    } else {
        // Waiting for opponent
        wattron(player2Win, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
        mvwprintw(player2Win, 3, (PLAYER_BOX_WIDTH - 7) / 2, "-------");
        mvwprintw(player2Win, 4, (PLAYER_BOX_WIDTH - 9) / 2, "< EMPTY >");
        mvwprintw(player2Win, 5, (PLAYER_BOX_WIDTH - 7) / 2, "-------");
        mvwprintw(player2Win, 6, (PLAYER_BOX_WIDTH - 11) / 2, "Waiting...");
        wattroff(player2Win, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
    }
    
    wnoutrefresh(player2Win);
}

void RoomScreen::drawMenu() {
    int startY = MENU_START_Y;
    int centerX = width / 2;
    
    // Different menu items for host vs guest
    const char* firstMenuItem;
    bool firstItemEnabled = true;
    
    if (isHost) {
        firstMenuItem = "[ START GAME ]";
        // Host can only start if guest is ready
        firstItemEnabled = guestReady && !guestName.empty();
    } else {
        firstMenuItem = "[ READY ]";
    }
    
    const char* menuItems[] = {
        firstMenuItem,
        "[ VIEW FREE USERS ]",
        "[ EXIT ROOM ]"
    };
    
    for (int i = 0; i < 3; i++) {
        RoomOption option = static_cast<RoomOption>(i);
        
        // Dim START button if guest not ready
        bool isDimmed = (i == 0 && isHost && !firstItemEnabled);
        
        if (selectedOption == option) {
            if (isDimmed) {
                wattron(mainWin, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
                mvwprintw(mainWin, startY, centerX - 15, ">  %-25s  <", menuItems[i]);
                wattroff(mainWin, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
            } else {
                wattron(mainWin, COLOR_PAIR(SELECTED_COLOR) | A_BOLD);
                mvwprintw(mainWin, startY, centerX - 15, ">  %-25s  <", menuItems[i]);
                wattroff(mainWin, COLOR_PAIR(SELECTED_COLOR) | A_BOLD);
            }
        } else {
            if (isDimmed) {
                wattron(mainWin, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
            } else {
                wattron(mainWin, COLOR_PAIR(NORMAL_COLOR));
            }
            mvwprintw(mainWin, startY, centerX - 15, "   %-25s   ", menuItems[i]);
            if (isDimmed) {
                wattroff(mainWin, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
            } else {
                wattroff(mainWin, COLOR_PAIR(NORMAL_COLOR));
            }
        }
        
        startY += 2;
    }
}

void RoomScreen::drawInstructions() {
    int y = height - 2;
    
    wattron(mainWin, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
    std::string inst;
    if (isHost) {
        inst = "[UP/DN: Navigate | Enter: Select | S: Start | E: Exit]";
    } else {
        inst = "[UP/DN: Navigate | Enter: Select | R: Ready | E: Exit]";
    }
    mvwprintw(mainWin, y, (width - inst.length()) / 2, "%s", inst.c_str());
    wattroff(mainWin, COLOR_PAIR(NORMAL_COLOR) | A_DIM);
}

void RoomScreen::draw() {
    // Clear and draw player boxes first (they're above mainWin)
    drawPlayerBoxes();
    
    // Draw room title on stdscr above player boxes
    wattron(stdscr, COLOR_PAIR(TITLE_COLOR) | A_BOLD);
    std::string title = "Room: " + roomName;
    mvwprintw(stdscr, 1, (width - title.length()) / 2, "%s", title.c_str());
    wattroff(stdscr, COLOR_PAIR(TITLE_COLOR) | A_BOLD);
    wnoutrefresh(stdscr);
    
    // Now draw main window below
    werase(mainWin);
    drawBorder();
    drawTitle();
    drawMenu();
    drawInstructions();
    wnoutrefresh(mainWin);
    
    doupdate();
}

void RoomScreen::handleNavigation(int ch) {
    switch(ch) {
        case KEY_UP:
            if (selectedOption == RoomOption::READY_OR_START) {
                selectedOption = RoomOption::EXIT_ROOM;
            } else {
                int current = static_cast<int>(selectedOption);
                selectedOption = static_cast<RoomOption>(current - 1);
            }
            break;
            
        case KEY_DOWN:
            if (selectedOption == RoomOption::EXIT_ROOM) {
                selectedOption = RoomOption::READY_OR_START;
            } else {
                int current = static_cast<int>(selectedOption);
                selectedOption = static_cast<RoomOption>(current + 1);
            }
            break;
    }
}

// Handles window resize events.
void RoomScreen::handleResize() {
    // Clean up old windows
    cleanupWindows();
    
    // Re-initialize windows with new dimensions
    initWindows();
    
    // Redraw the entire screen
    draw();
    touchwin(stdscr);
    refresh();
}

int RoomScreen::handleInput() {
    // Ensure window is ready for input
    touchwin(mainWin);
    wrefresh(mainWin);
    
    int ch = wgetch(mainWin);
    
    // Only log actual input, not timeouts
    if (ch != ERR && ch != KEY_RESIZE) {
    }
    
    // Handle timeout (ERR means no input available)
    if (ch == ERR) {
        return 0; // No input, continue
    }
    
    // Handle window resize
    if (ch == KEY_RESIZE) {
        handleResize();
        return 3; // Special value to indicate resize
    }

    if (ch == KEY_UP || ch == KEY_DOWN) {
        handleNavigation(ch);
        return 0;
    }
    
    if (ch == 10 || ch == KEY_ENTER) {
        switch(selectedOption) {
            case RoomOption::READY_OR_START:
                // Return 1 for both Ready (guest) and Start (host)
                // Host can only start if guest is ready
                if (isHost && (!guestReady || guestName.empty())) {
                    return 0; // Can't start yet
                }
                return 1;
            case RoomOption::VIEW_FREE_USERS:
                return 2;
            case RoomOption::EXIT_ROOM:
                return -1;
        }
    }
    
    // Keyboard shortcuts
    switch(ch) {
        case 'r':
        case 'R':
            if (!isHost) {
                return 1;  // Ready (guest only)
            }
            break;
        case 's':
        case 'S':
            if (isHost && guestReady && !guestName.empty()) {
                return 1;  // Start (host only, if guest ready)
            }
            break;
        case 'v':
        case 'V':
            return 2;  // View users
        case 'e':
        case 'E':
        case 27:  // ESC
            return -1; // Exit
    }
    
    return 0;  // Continue
}

void RoomScreen::setGuestPlayer(const std::string& name) {
    guestName = name;
}

void RoomScreen::removeGuestPlayer() {
    guestName = "";
    guestReady = false;
}

void RoomScreen::setHostReady(bool ready) {
    hostReady = ready;
}

void RoomScreen::setGuestReady(bool ready) {
    guestReady = ready;
}

void RoomScreen::setRoomName(const std::string& name) {
    roomName = name;
}
