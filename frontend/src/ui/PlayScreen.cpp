#include "ui/PlayScreen.h"
#include <ncurses.h>
#include <algorithm>
#include <sstream>

namespace hangman {

PlayScreen::PlayScreen(const std::string& roomName,
                       const std::string& hostUsername,
                       const std::string& guestUsername,
                       const std::string& currentUsername,
                       bool isHost,
                       uint32_t roomId,
                       uint32_t matchId,
                       int wordLength)
    : roomName(roomName),
      hostUsername(hostUsername),
      guestUsername(guestUsername),
      currentUsername(currentUsername),
      isHost(isHost),
      roomId(roomId),
      matchId(matchId),
      remainingAttempts(6),
      wordLength(wordLength),
      isMyTurn(isHost),  // Host starts first
      gameOver(false),
      iWon(false),
      inputMode(InputMode::NORMAL),
      selectedMenuOption(0) {
    
    // Initialize word pattern with blanks
    wordPattern = "";
    for (int i = 0; i < wordLength; i++) {
        wordPattern += "_ ";
    }
    if (!wordPattern.empty()) wordPattern.pop_back();
    
    gameMessage = "Game started! Good luck!";
}

void PlayScreen::drawHangman(int y, int x, int wrongGuesses) {
    // Draw gallows base
    attron(COLOR_PAIR(1));
    mvprintw(y,     x, "    +-------+");
    mvprintw(y + 1, x, "    |       |");
    
    // Draw hangman parts based on wrong guesses
    if (wrongGuesses >= 1) mvprintw(y + 2, x, "    |       O");      // Head
    else                    mvprintw(y + 2, x, "    |        ");
    
    if (wrongGuesses >= 4) mvprintw(y + 3, x, "    |      /|\\");    // Body + arms
    else if (wrongGuesses >= 3) mvprintw(y + 3, x, "    |      /|");
    else if (wrongGuesses >= 2) mvprintw(y + 3, x, "    |       |");
    else                        mvprintw(y + 3, x, "    |        ");
    
    if (wrongGuesses >= 6) mvprintw(y + 4, x, "    |      / \\");    // Legs
    else if (wrongGuesses >= 5) mvprintw(y + 4, x, "    |      /");
    else                        mvprintw(y + 4, x, "    |        ");
    
    mvprintw(y + 5, x, "    |");
    mvprintw(y + 6, x, "   ===");
    attroff(COLOR_PAIR(1));
    
    // Lives indicator
    attron(COLOR_PAIR(wrongGuesses >= 4 ? 1 : 2));
    mvprintw(y + 8, x, "   Lives: %d/6", remainingAttempts);
    attroff(COLOR_PAIR(wrongGuesses >= 4 ? 1 : 2));
}

void PlayScreen::drawWordPattern(int y, int x) {
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(y, x, "Word: ");
    
    // Make letters bigger with spacing
    std::string display = "";
    for (char c : wordPattern) {
        if (c == ' ') display += "  ";
        else display += c;
        display += " ";
    }
    
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(y, x + 6, "%s", display.c_str());
    attroff(COLOR_PAIR(3) | A_BOLD);
    attroff(COLOR_PAIR(2) | A_BOLD);
}

void PlayScreen::drawGuessedLetters(int y, int x) {
    attron(COLOR_PAIR(4));
    mvprintw(y, x, "Guessed: ");
    
    std::string guessedStr = "";
    for (char c : guessedChars) {
        guessedStr += c;
        guessedStr += " ";
    }
    if (guessedStr.empty()) guessedStr = "(none)";
    
    mvprintw(y, x + 9, "%s", guessedStr.c_str());
    attroff(COLOR_PAIR(4));
}

void PlayScreen::drawGameInfo(int y, int x) {
    // Room name
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(y, x, "Room: %s", roomName.c_str());
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    // Players
    attron(COLOR_PAIR(4));
    mvprintw(y + 2, x, "Host:  %s %s", hostUsername.c_str(), 
             (hostUsername == currentUsername) ? "(You)" : "");
    mvprintw(y + 3, x, "Guest: %s %s", guestUsername.c_str(),
             (guestUsername == currentUsername) ? "(You)" : "");
    attroff(COLOR_PAIR(4));
    
    // Turn indicator
    y += 5;
    if (isMyTurn) {
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(y, x, ">>> YOUR TURN <<<");
        attroff(COLOR_PAIR(2) | A_BOLD);
    } else {
        attron(COLOR_PAIR(6));
        mvprintw(y, x, "Opponent's turn...");
        attroff(COLOR_PAIR(6));
    }
    
    // Game message
    if (!gameMessage.empty()) {
        attron(COLOR_PAIR(5));
        mvprintw(y + 2, x, "%s", gameMessage.c_str());
        attroff(COLOR_PAIR(5));
    }
}

void PlayScreen::drawOptionsMenu(int y, int x) {
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(y, x, "+-------------------------+");
    mvprintw(y + 1, x, "|      GAME OPTIONS       |");
    mvprintw(y + 2, x, "+-------------------------+");
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    const char* options[] = {
        "Resume Game",
        "Request Draw",
        "Surrender",
        "Exit Game"
    };
    
    for (int i = 0; i < 4; i++) {
        if (i == selectedMenuOption) {
            attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
        } else {
            attron(COLOR_PAIR(4));
        }
        mvprintw(y + 4 + i, x + 2, "%s", options[i]);
        if (i == selectedMenuOption) {
            attroff(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
        } else {
            attroff(COLOR_PAIR(4));
        }
    }
    
    attron(COLOR_PAIR(6));
    mvprintw(y + 9, x, "[UP/DOWN] Navigate");
    mvprintw(y + 10, x, "[ENTER] Select [ESC] Back");
    attroff(COLOR_PAIR(6));
}

void PlayScreen::drawInputPrompt() {
    int promptY = LINES - 4;
    
    if (inputMode == InputMode::GUESS_CHAR) {
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(promptY, 2, "Press a letter to guess (ESC to cancel): ");
        attroff(COLOR_PAIR(2) | A_BOLD);
    } else if (inputMode == InputMode::GUESS_WORD) {
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(promptY, 2, "Enter word guess: %s_", wordInput.c_str());
        attroff(COLOR_PAIR(2) | A_BOLD);
        attron(COLOR_PAIR(6));
        mvprintw(promptY + 1, 2, "(Press ENTER to submit, ESC to cancel)");
        attroff(COLOR_PAIR(6));
    }
}

void PlayScreen::drawGameOverScreen() {
    int centerY = LINES / 2 - 5;
    int centerX = COLS / 2;
    
    // Draw semi-transparent overlay effect
    attron(COLOR_PAIR(1) | A_BOLD);
    for (int i = centerY - 2; i < centerY + 8; i++) {
        mvprintw(i, centerX - 25, "%50s", " ");
    }
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    // Game Over banner
    if (iWon) {
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(centerY, centerX - 15, "+----------------------------+");
        mvprintw(centerY + 1, centerX - 15, "|   YOU WIN! CONGRATULATIONS! |");
        mvprintw(centerY + 2, centerX - 15, "+----------------------------+");
        attroff(COLOR_PAIR(2) | A_BOLD);
    } else {
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(centerY, centerX - 15, "+----------------------------+");
        mvprintw(centerY + 1, centerX - 15, "|      GAME OVER - YOU LOST   |");
        mvprintw(centerY + 2, centerX - 15, "+----------------------------+");
        attroff(COLOR_PAIR(1) | A_BOLD);
    }
    
    // Game message
    attron(COLOR_PAIR(5));
    mvprintw(centerY + 4, centerX - gameMessage.length() / 2, "%s", gameMessage.c_str());
    attroff(COLOR_PAIR(5));
    
    // Exit instruction
    attron(COLOR_PAIR(6));
    mvprintw(centerY + 6, centerX - 20, "Press any key to return to menu...");
    attroff(COLOR_PAIR(6));
}

void PlayScreen::draw() {
    clear();
    
    if (gameOver) {
        drawGameOverScreen();
        refresh();
        return;
    }
    
    if (inputMode == InputMode::MENU) {
        drawOptionsMenu(LINES / 2 - 6, COLS / 2 - 13);
        refresh();
        return;
    }
    
    // Title
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(1, (COLS - 30) / 2, "===  HANGMAN GAME  ===");
    attroff(COLOR_PAIR(2) | A_BOLD);
    
    // Layout: Left side = Hangman, Right side = Game Info
    int leftX = 5;
    int rightX = COLS / 2 + 5;
    int topY = 4;
    
    // Left panel - Hangman
    int wrongGuesses = 6 - remainingAttempts;
    drawHangman(topY, leftX, wrongGuesses);
    
    // Right panel - Game Info
    drawGameInfo(topY, rightX);
    
    // Center - Word and guessed letters
    int wordY = topY + 11;
    drawWordPattern(wordY, (COLS - 40) / 2);
    drawGuessedLetters(wordY + 2, (COLS - 40) / 2);
    
    // Bottom - Controls
    int controlsY = LINES - 7;
    attron(COLOR_PAIR(6));
    mvprintw(controlsY, (COLS - 60) / 2, "+---------------------------------------------------------+");
    mvprintw(controlsY + 1, (COLS - 60) / 2, "|  [C] Guess Character  |  [W] Guess Word  |  [M] Menu  |");
    mvprintw(controlsY + 2, (COLS - 60) / 2, "+---------------------------------------------------------+");
    attroff(COLOR_PAIR(6));
    
    // Input prompt if active
    if (inputMode != InputMode::NORMAL) {
        drawInputPrompt();
    }
    
    refresh();
}

int PlayScreen::handleInput() {
    if (gameOver) {
        getch();  // Wait for any key
        return -1;  // Exit to menu
    }
    
    int ch = getch();
    
    // Handle different input modes
    switch (inputMode) {
        case InputMode::MENU:
            switch (ch) {
                case KEY_UP:
                    selectedMenuOption = (selectedMenuOption - 1 + 4) % 4;
                    break;
                case KEY_DOWN:
                    selectedMenuOption = (selectedMenuOption + 1) % 4;
                    break;
                case 10:  // ENTER
                case 13:
                    switch (selectedMenuOption) {
                        case 0:  // Resume
                            inputMode = InputMode::NORMAL;
                            break;
                        case 1:  // Request Draw
                            // TODO: Send draw request packet
                            gameMessage = "Draw request sent!";
                            inputMode = InputMode::NORMAL;
                            break;
                        case 2:  // Surrender
                            // TODO: Send surrender packet
                            setGameOver(false, "You surrendered");
                            break;
                        case 3:  // Exit
                            return -1;
                    }
                    break;
                case 27:  // ESC
                    inputMode = InputMode::NORMAL;
                    break;
            }
            break;
            
        case InputMode::GUESS_CHAR:
            if (ch == 27) {  // ESC
                inputMode = InputMode::NORMAL;
                gameMessage = "";
            } else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
                char guess = toupper(ch);
                
                // Check if already guessed
                if (guessedChars.count(guess)) {
                    gameMessage = "Already guessed that letter!";
                } else {
                    // TODO: Send C2S_GuessChar packet
                    guessedChars.insert(guess);
                    gameMessage = std::string("Guessed: ") + guess;
                    inputMode = InputMode::NORMAL;
                    // Server will respond with result
                }
            }
            break;
            
        case InputMode::GUESS_WORD:
            if (ch == 27) {  // ESC
                inputMode = InputMode::NORMAL;
                wordInput.clear();
                gameMessage = "";
            } else if (ch == 10 || ch == 13) {  // ENTER
                if (!wordInput.empty()) {
                    // TODO: Send C2S_GuessWord packet
                    std::string guess = wordInput;
                    wordInput.clear();
                    inputMode = InputMode::NORMAL;
                    gameMessage = "Word guessed: " + guess;
                    // Server will respond with result
                }
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                if (!wordInput.empty()) {
                    wordInput.pop_back();
                }
            } else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
                if (wordInput.length() < 20) {  // Max word length
                    wordInput += toupper(ch);
                }
            }
            break;
            
        case InputMode::NORMAL:
            switch (ch) {
                case 'c':
                case 'C':
                    if (isMyTurn && !gameOver) {
                        inputMode = InputMode::GUESS_CHAR;
                        gameMessage = "";
                    } else if (!isMyTurn) {
                        gameMessage = "Wait for your turn!";
                    }
                    break;
                    
                case 'w':
                case 'W':
                    if (isMyTurn && !gameOver) {
                        inputMode = InputMode::GUESS_WORD;
                        wordInput.clear();
                        gameMessage = "";
                    } else if (!isMyTurn) {
                        gameMessage = "Wait for your turn!";
                    }
                    break;
                    
                case 'm':
                case 'M':
                    inputMode = InputMode::MENU;
                    selectedMenuOption = 0;
                    break;
                    
                case 27:  // ESC - Quick exit
                    inputMode = InputMode::MENU;
                    selectedMenuOption = 3;  // Exit option
                    break;
            }
            break;
    }
    
    return 0;
}

// Game state update methods
void PlayScreen::updateWordPattern(const std::string& pattern) {
    wordPattern = pattern;
}

void PlayScreen::addGuessedChar(char ch) {
    guessedChars.insert(toupper(ch));
}

void PlayScreen::setRemainingAttempts(int attempts) {
    remainingAttempts = attempts;
}

void PlayScreen::setMyTurn(bool myTurn) {
    isMyTurn = myTurn;
}

void PlayScreen::setGameOver(bool won, const std::string& message) {
    gameOver = true;
    iWon = won;
    gameMessage = message;
}

void PlayScreen::setGameMessage(const std::string& msg) {
    gameMessage = msg;
}

} // namespace hangman
