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
      currentScore(0),
      currentRound(1),
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
    
    gameMessage = "Round 1 - Good luck!";
    
    // Initialize input with timeout for real-time updates
    initInput();
}

void PlayScreen::initInput() {
    // halfdelay mode: getch() waits max 1 decisecond (100ms)
    halfdelay(1);  // 1 decisecond = 100ms
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
    
    // Round and Score
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(y + 1, x, "Round: %d/2  |  Score: %u", currentRound, currentScore);
    attroff(COLOR_PAIR(2) | A_BOLD);
    
    // Players
    attron(COLOR_PAIR(4));
    mvprintw(y + 3, x, "Host:  %s %s", hostUsername.c_str(), 
             (hostUsername == currentUsername) ? "(You)" : "");
    mvprintw(y + 4, x, "Guest: %s %s", guestUsername.c_str(),
             (guestUsername == currentUsername) ? "(You)" : "");
    attroff(COLOR_PAIR(4));
    
    // Turn indicator
    y += 6;
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
        cbreak();  // Reset to normal mode
        getch();  // Wait for any key
        return -1;  // Exit to menu
    }
    
    int ch = getch();
    
    // Handle timeout (ERR means no input within timeout period)
    if (ch == ERR) {
        return 0;  // No input, just continue (allows notifications to be processed)
    }
    
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
                            try {
                                auto& client = GameClient::getInstance();
                                client.requestDraw(roomId, matchId);
                                gameMessage = "Draw request sent to opponent!";
                                inputMode = InputMode::NORMAL;
                            } catch (const std::exception& e) {
                                gameMessage = std::string("Error: ") + e.what();
                            }
                            break;
                        case 2:  // Surrender
                            try {
                                auto& client = GameClient::getInstance();
                                // result_code: 0 = resignation
                                client.endGame(roomId, matchId, 0, "Player surrendered");
                                setGameOver(false, "You surrendered");
                            } catch (const std::exception& e) {
                                gameMessage = std::string("Error: ") + e.what();
                            }
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
                    // Send C2S_GuessChar packet
                    try {
                        auto& client = GameClient::getInstance();
                        auto response = client.guessChar(roomId, matchId, guess);
                        
                        guessedChars.insert(guess);
                        
                        uint8_t oldRound = currentRound;
                        updateWordPattern(response.exposed_pattern);
                        setRemainingAttempts(response.remaining_attempts);
                        setScore(response.total_score);
                        
                        // Check if word is complete in OLD round (before server transitioned)
                        bool wordCompleteInOldRound = true;
                        for (char c : response.exposed_pattern) {
                            if (c == '_') {
                                wordCompleteInOldRound = false;
                                break;
                            }
                        }
                        
                        // Set initial message based on guess result
                        if (response.correct) {
                            gameMessage = "Correct! +" + std::to_string(response.score_gained) + " points! " + guess + " is in the word!";
                        } else {
                            gameMessage = std::string("Wrong! ") + guess + " is not in the word.";
                        }
                        
                        // Check if round changed (server handles round transition)
                        if (response.current_round > oldRound) {
                            // Round transition happened
                            if (wordCompleteInOldRound) {
                                // We completed the word in old round
                                gameMessage = "You completed Round " + std::to_string((int)oldRound) + "! Score: " + std::to_string(response.total_score) + ". Moving to Round " + std::to_string((int)response.current_round) + "...";
                            } else {
                                // We ran out of attempts in old round
                                gameMessage = "Round " + std::to_string((int)oldRound) + " over. Moving to Round " + std::to_string((int)response.current_round) + "...";
                            }
                            
                            // Update round AFTER showing completion message
                            setRound(response.current_round);
                            
                            // Draw once to show the completion message
                            draw();
                            napms(2000);  // Pause 2 seconds to show message
                            
                            // Now transition to new round
                            handleRoundTransition(response.exposed_pattern);
                            gameMessage = "Round " + std::to_string((int)response.current_round) + " started!";
                            inputMode = InputMode::NORMAL;
                            isMyTurn = true;  // Continue playing
                        } else {
                            // No round change - update round normally
                            setRound(response.current_round);
                            
                            if (wordCompleteInOldRound && response.current_round == 2) {
                                // Completed round 2 - game over
                                setGameOver(true, "You won both rounds! Final score: " + std::to_string(response.total_score));
                            } else if (response.remaining_attempts == 0 && response.current_round == 2) {
                                // Out of attempts in round 2
                                setGameOver(false, "Out of attempts! Final score: " + std::to_string(response.total_score));
                            } else {
                                inputMode = InputMode::NORMAL;
                                isMyTurn = false;  // Turn switches to opponent
                            }
                        }
                    } catch (const std::exception& e) {
                        gameMessage = std::string("Error: ") + e.what();
                    }
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
                    // Send C2S_GuessWord packet
                    try {
                        auto& client = GameClient::getInstance();
                        std::string guess = wordInput;
                        auto response = client.guessWord(roomId, matchId, guess);
                        
                        wordInput.clear();
                        inputMode = InputMode::NORMAL;
                        
                        uint8_t oldRound = currentRound;
                        setScore(response.total_score);
                        
                        if (response.round_complete) {
                            // Round transition - show completion message first
                            gameMessage = "You completed Round " + std::to_string((int)oldRound) + "! Score: " + std::to_string(response.total_score) + ". Moving to Round " + std::to_string((int)response.current_round) + "...";
                            
                            // Draw once to show the message
                            draw();
                            napms(2000);  // Pause 2 seconds
                            
                            // Now transition
                            setRound(response.current_round);
                            handleRoundTransition(response.next_word_pattern);
                            gameMessage = "Round " + std::to_string((int)response.current_round) + " started!";
                            isMyTurn = true;  // Continue playing in next round
                        } else {
                            setRound(response.current_round);
                            
                            if (response.correct && response.current_round == 2) {
                                // Correct guess in round 2 - game over
                                setGameOver(true, "You guessed the word: " + guess + "! Final score: " + std::to_string(response.total_score));
                            } else if (response.correct && response.current_round == 1) {
                                // This shouldn't happen (server should set round_complete), but handle it
                                handleRoundTransition(response.next_word_pattern);
                                gameMessage = "Correct! Moving to Round 2!";
                                isMyTurn = true;
                            } else {
                                setRemainingAttempts(response.remaining_attempts);
                                gameMessage = response.message;
                                
                                if (response.remaining_attempts == 0 && response.current_round == 2) {
                                    setGameOver(false, "Out of attempts! Final score: " + std::to_string(response.total_score));
                                } else if (response.remaining_attempts == 0 && response.current_round == 1) {
                                    // This shouldn't happen (server should set round_complete), but handle it
                                    handleRoundTransition(response.next_word_pattern);
                                    gameMessage = "Out of attempts! Moving to Round 2.";
                                    isMyTurn = true;
                                } else {
                                    isMyTurn = false;  // Turn switches to opponent
                                }
                            }
                        }
                    } catch (const std::exception& e) {
                        gameMessage = std::string("Error: ") + e.what();
                    }
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

void PlayScreen::setScore(uint32_t score) {
    currentScore = score;
}

void PlayScreen::setRound(uint8_t round) {
    currentRound = round;
}

void PlayScreen::handleRoundTransition(const std::string& newPattern) {
    // Clear guessed chars for new round
    guessedChars.clear();
    // Update word pattern
    wordPattern = newPattern;
    // Reset attempts
    remainingAttempts = 6;
    // Update game message
    gameMessage = "Round " + std::to_string(currentRound) + " started!";
}

void PlayScreen::processNotifications() {
    // These are declared in test.cpp as extern or we need access
    // For now, we'll handle this in the main game loop in test.cpp
    // This method is a placeholder that can be called from test.cpp
}

} // namespace hangman
