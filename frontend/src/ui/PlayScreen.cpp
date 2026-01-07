#include "ui/PlayScreen.h"
#include <ncurses.h>

namespace hangman {

PlayScreen::PlayScreen(const std::string& roomName,
                       const std::string& hostUsername,
                       const std::string& guestUsername,
                       const std::string& currentUsername,
                       bool isHost)
    : roomName(roomName),
      hostUsername(hostUsername),
      guestUsername(guestUsername),
      currentUsername(currentUsername),
      isHost(isHost) {
}

void PlayScreen::draw() {
    clear();
    
    // Title with simple ASCII
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(2, (COLS - 44) / 2, "+------------------------------------------+");
    mvprintw(3, (COLS - 44) / 2, "|       HANGMAN GAME - IN PROGRESS        |");
    mvprintw(4, (COLS - 44) / 2, "+------------------------------------------+");
    attroff(COLOR_PAIR(2) | A_BOLD);
    
    // Room info
    attron(COLOR_PAIR(3));
    std::string roomInfo = "Room: " + roomName;
    mvprintw(6, (COLS - roomInfo.length()) / 2, "%s", roomInfo.c_str());
    attroff(COLOR_PAIR(3));
    
    // Players info
    int centerY = LINES / 2 - 5;
    
    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(centerY, COLS / 4 - 5, "HOST: %s", hostUsername.c_str());
    mvprintw(centerY, 3 * COLS / 4 - 5, "GUEST: %s", guestUsername.c_str());
    attroff(COLOR_PAIR(4) | A_BOLD);
    
    // Hangman display area
    int hangmanY = centerY + 3;
    attron(COLOR_PAIR(1));
    mvprintw(hangmanY,     COLS / 2 - 10, "+-------------------+");
    mvprintw(hangmanY + 1, COLS / 2 - 10, "|                   |");
    mvprintw(hangmanY + 2, COLS / 2 - 10, "|   HANGMAN AREA    |");
    mvprintw(hangmanY + 3, COLS / 2 - 10, "|   (Coming Soon)   |");
    mvprintw(hangmanY + 4, COLS / 2 - 10, "|                   |");
    mvprintw(hangmanY + 5, COLS / 2 - 10, "+-------------------+");
    attroff(COLOR_PAIR(1));
    
    // Word display area
    int wordY = hangmanY + 8;
    attron(COLOR_PAIR(2));
    mvprintw(wordY, COLS / 2 - 15, "Word: _ _ _ _ _ _ _ _");
    attroff(COLOR_PAIR(2));
    
    // Guessed letters
    int guessY = wordY + 2;
    attron(COLOR_PAIR(3));
    mvprintw(guessY, COLS / 2 - 15, "Guessed: A B C D E");
    attroff(COLOR_PAIR(3));
    
    // Turn indicator
    int turnY = guessY + 3;
    attron(COLOR_PAIR(5) | A_BOLD);
    if (isHost) {
        mvprintw(turnY, COLS / 2 - 10, ">>> YOUR TURN <<<");
    } else {
        mvprintw(turnY, COLS / 2 - 15, "Waiting for opponent...");
    }
    attroff(COLOR_PAIR(5) | A_BOLD);
    
    // Instructions
    attron(COLOR_PAIR(6));
    mvprintw(LINES - 3, (COLS - 40) / 2, "[A-Z] Guess Letter  |  [ESC] Exit Game");
    attroff(COLOR_PAIR(6));
    
    refresh();
}

int PlayScreen::handleInput() {
    int ch = getch();
    
    switch (ch) {
        case 27:  // ESC
            return -1;
        case 'a': case 'A':
        case 'b': case 'B':
        case 'c': case 'C':
        case 'd': case 'D':
        case 'e': case 'E':
        case 'f': case 'F':
        case 'g': case 'G':
        case 'h': case 'H':
        case 'i': case 'I':
        case 'j': case 'J':
        case 'k': case 'K':
        case 'l': case 'L':
        case 'm': case 'M':
        case 'n': case 'N':
        case 'o': case 'O':
        case 'p': case 'P':
        case 'q': case 'Q':
        case 'r': case 'R':
        case 's': case 'S':
        case 't': case 'T':
        case 'u': case 'U':
        case 'v': case 'V':
        case 'w': case 'W':
        case 'x': case 'X':
        case 'y': case 'Y':
        case 'z': case 'Z':
            // TODO: Handle letter guess
            return 0;
        default:
            return 0;
    }
}

} // namespace hangman
