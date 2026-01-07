#include "ui/GameScreen.h"
#include <algorithm>
#include <cctype>

GameScreen::GameScreen()
    : currentRound(1),
      secretWord(""),
      exposedPattern(""),
      isMyTurn(false),
      remainingAttempts(7),
      hangmanStage(0),
      inputBuffer(""),
      statusMessage(""),
      selectedButton(GameButton::FORFEIT) {
    
    getmaxyx(stdscr, height, width);
    mainWin = newwin(height, width, 0, 0);
    keypad(mainWin, TRUE);
    timeout(100);
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
}

GameScreen::~GameScreen() {
    if (mainWin) delwin(mainWin);
}

int GameScreen::getCharPoints() const {
    switch(currentRound) {
        case 1: return 10;
        case 2: return 15;
        case 3: return 20;
        default: return 10;
    }
}

int GameScreen::getWordCorrectPoints() const {
    switch(currentRound) {
        case 1: return 30;
        case 2: return 50;
        case 3: return 80;
        default: return 30;
    }
}

int GameScreen::getWordWrongPenalty() const {
    switch(currentRound) {
        case 1: return 10;
        case 2: return 15;
        case 3: return 15;
        default: return 10;
    }
}

void GameScreen::drawBorder() {
    box(mainWin, 0, 0);
}

void GameScreen::drawRoundInfo() {
    wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(mainWin, 1, 3, "ROUND %d / 3", currentRound);
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
}

void GameScreen::drawScores() {
    int rightX = width - 25;
    
    // Player 1 score
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, 1, rightX, "%s:", player1.username.c_str());
    wattroff(mainWin, COLOR_PAIR(3));
    
    wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(mainWin, 1, rightX + player1.username.length() + 2, "%d", player1.score);
    wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
    
    // Player 2 score
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, 2, rightX, "%s:", player2.username.c_str());
    wattroff(mainWin, COLOR_PAIR(3));
    
    wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(mainWin, 2, rightX + player2.username.length() + 2, "%d", player2.score);
    wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
}

void GameScreen::drawHangmanStage(int stage) {
    int startY = 5;
    int startX = 5;
    
    wattron(mainWin, COLOR_PAIR(4) | A_BOLD);
    
    // Gallows (always visible)
    mvwprintw(mainWin, startY, startX, "  +---+");
    mvwprintw(mainWin, startY + 1, startX, "  |   |");
    
    // Stage 0: empty
    if (stage >= 1) mvwprintw(mainWin, startY + 2, startX, "  |   O");      // Head
    else mvwprintw(mainWin, startY + 2, startX, "  |    ");
    
    if (stage >= 4) mvwprintw(mainWin, startY + 3, startX, "  |  /|\\");    // Body + arms
    else if (stage >= 3) mvwprintw(mainWin, startY + 3, startX, "  |  /|");  // Body + left arm
    else if (stage >= 2) mvwprintw(mainWin, startY + 3, startX, "  |   |");  // Body only
    else mvwprintw(mainWin, startY + 3, startX, "  |    ");
    
    if (stage >= 6) mvwprintw(mainWin, startY + 4, startX, "  |  / \\");    // Both legs
    else if (stage >= 5) mvwprintw(mainWin, startY + 4, startX, "  |  /");   // Left leg
    else mvwprintw(mainWin, startY + 4, startX, "  |    ");
    
    mvwprintw(mainWin, startY + 5, startX, "  |");
    mvwprintw(mainWin, startY + 6, startX, "=====");
    
    if (stage >= 7) {
        mvwprintw(mainWin, startY + 7, startX, " DEAD!");
    }
    
    wattroff(mainWin, COLOR_PAIR(4) | A_BOLD);
}

void GameScreen::drawHangman() {
    drawHangmanStage(hangmanStage);
}

void GameScreen::drawSecretWord() {
    int startY = 6;
    int startX = width / 2 + 5;
    
    wattron(mainWin, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(mainWin, startY, startX, "SECRET WORD:");
    wattroff(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    // Draw exposed pattern with large letters
    wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
    std::string displayPattern = "";
    for (char c : exposedPattern) {
        if (c == '_') {
            displayPattern += "_ ";
        } else if (c == ' ') {
            displayPattern += "  ";
        } else {
            displayPattern += c;
            displayPattern += " ";
        }
    }
    mvwprintw(mainWin, startY + 2, startX, "%s", displayPattern.c_str());
    wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
}

void GameScreen::drawAlphabet() {
    int startY = 10;
    int startX = width / 2 + 5;
    
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, startY, startX, "ALPHABET:");
    wattroff(mainWin, COLOR_PAIR(3));
    
    // Draw A-Z in rows
    std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int row = 0;
    for (size_t i = 0; i < alphabet.length(); i++) {
        char c = alphabet[i];
        int col = i % 13;
        row = i / 13;
        
        if (guessedLetters.count(c) > 0) {
            wattron(mainWin, COLOR_PAIR(3) | A_DIM);
        } else {
            wattron(mainWin, COLOR_PAIR(2));
        }
        
        mvwprintw(mainWin, startY + 2 + row, startX + col * 2, "%c", c);
        
        wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
        wattroff(mainWin, COLOR_PAIR(2));
    }
}

void GameScreen::drawTurnInfo() {
    int startY = height - 12;
    int startX = 3;
    
    wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    if (isMyTurn) {
        mvwprintw(mainWin, startY, startX, ">>> YOUR TURN <<<");
    } else {
        mvwprintw(mainWin, startY, startX, "Opponent's turn...");
    }
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, startY + 1, startX, "Attempts left: %d/7", remainingAttempts);
    wattroff(mainWin, COLOR_PAIR(3));
}

void GameScreen::drawInputArea() {
    int startY = height - 9;
    int startX = 3;
    
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, startY, startX, "Your guess:");
    wattroff(mainWin, COLOR_PAIR(3));
    
    // Input box
    wattron(mainWin, COLOR_PAIR(5));
    mvwprintw(mainWin, startY, startX + 13, "[%-20s]", inputBuffer.c_str());
    wattroff(mainWin, COLOR_PAIR(5));
    
    // Status message
    if (!statusMessage.empty()) {
        int color = (statusMessage.find("Error") != std::string::npos ||
                     statusMessage.find("Wrong") != std::string::npos ||
                     statusMessage.find("Invalid") != std::string::npos) ? 4 : 2;
        
        wattron(mainWin, COLOR_PAIR(color));
        mvwprintw(mainWin, startY + 2, startX, "%s", statusMessage.c_str());
        wattroff(mainWin, COLOR_PAIR(color));
    }
}

void GameScreen::drawButtons() {
    int buttonY = height - 4;
    int startX = 3;
    
    // Forfeit button
    if (selectedButton == GameButton::FORFEIT) {
        wattron(mainWin, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
        mvwprintw(mainWin, buttonY, startX, " >> [ FORFEIT THIS ROUND ] << ");
        wattroff(mainWin, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
    } else {
        wattron(mainWin, COLOR_PAIR(6));
        mvwprintw(mainWin, buttonY, startX, "    [ FORFEIT THIS ROUND ]    ");
        wattroff(mainWin, COLOR_PAIR(6));
    }
    
    // Quit button
    if (selectedButton == GameButton::QUIT) {
        wattron(mainWin, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
        mvwprintw(mainWin, buttonY, startX + 35, " >> [ QUIT GAME ] << ");
        wattroff(mainWin, COLOR_PAIR(4) | A_BOLD | A_REVERSE);
    } else {
        wattron(mainWin, COLOR_PAIR(4));
        mvwprintw(mainWin, buttonY, startX + 35, "    [ QUIT GAME ]    ");
        wattroff(mainWin, COLOR_PAIR(4));
    }
}

void GameScreen::drawRecentGuesses() {
    int startY = 5;
    int startX = width - 35;
    
    wattron(mainWin, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(mainWin, startY, startX, "RECENT GUESSES:");
    wattroff(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    // Draw last 5 guesses
    int displayed = 0;
    for (auto it = recentGuesses.rbegin(); it != recentGuesses.rend() && displayed < 5; ++it, ++displayed) {
        int color = it->correct ? 2 : 4;
        wattron(mainWin, COLOR_PAIR(color));
        
        std::string guessText = it->player + ": " + it->guess;
        if (guessText.length() > 30) {
            guessText = guessText.substr(0, 27) + "...";
        }
        
        mvwprintw(mainWin, startY + 2 + displayed, startX, "%-30s", guessText.c_str());
        wattroff(mainWin, COLOR_PAIR(color));
    }
}

void GameScreen::draw() {
    werase(mainWin);
    drawBorder();
    drawRoundInfo();
    drawScores();
    drawHangman();
    drawSecretWord();
    drawAlphabet();
    drawTurnInfo();
    drawInputArea();
    drawButtons();
    drawRecentGuesses();
    wrefresh(mainWin);
}

void GameScreen::handleInput(int ch) {
    if (ch == ERR) return;
    
    // Tab to switch between buttons
    if (ch == 9) {
        selectedButton = (selectedButton == GameButton::FORFEIT) ? 
                         GameButton::QUIT : GameButton::FORFEIT;
        return;
    }
    
    // Handle input for guess
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (!inputBuffer.empty()) {
            inputBuffer.pop_back();
            statusMessage = "";
        }
    } else if (ch == 10 || ch == KEY_ENTER) {
        processGuess();
    } else if (ch >= 32 && ch <= 126 && inputBuffer.length() < 20) {
        inputBuffer += static_cast<char>(ch);
        statusMessage = "";
    }
}

void GameScreen::processGuess() {
    if (inputBuffer.empty()) {
        statusMessage = "Error: Please enter a guess!";
        return;
    }
    
    // Check if it's a character guess (1 letter)
    if (inputBuffer.length() == 1) {
        // Character guess - must be my turn
        if (!isMyTurn) {
            statusMessage = "Error: Not your turn to guess a letter!";
            return;
        }
        
        char ch = std::toupper(inputBuffer[0]);
        
        if (!std::isalpha(ch)) {
            statusMessage = "Error: Please enter a letter A-Z!";
            return;
        }
        
        if (guessedLetters.count(ch) > 0) {
            statusMessage = "Error: Letter already guessed!";
            return;
        }
        
        // Valid character guess
        guessedLetters.insert(ch);
        addGuessToHistory(currentUser, std::string(1, ch), false);  // Will update after server response
        statusMessage = "Guessing letter: " + std::string(1, ch);
        inputBuffer = "";
        
    } else {
        // Word guess - can be done anytime
        std::string guess = inputBuffer;
        std::transform(guess.begin(), guess.end(), guess.begin(), ::toupper);
        
        addGuessToHistory(currentUser, guess, false);  // Will update after server response
        statusMessage = "Guessing word: " + guess;
        inputBuffer = "";
    }
}

void GameScreen::addGuessToHistory(const std::string& player, const std::string& guess, bool correct) {
    recentGuesses.push_back(GuessHistory(player, guess, correct));
    if (recentGuesses.size() > 5) {
        recentGuesses.erase(recentGuesses.begin());
    }
}

int GameScreen::update() {
    int ch = wgetch(mainWin);
    handleInput(ch);
    
    // Check button selections
    if (ch == 10 || ch == KEY_ENTER) {
        if (inputBuffer.empty()) {  // No input, check buttons
            if (selectedButton == GameButton::FORFEIT) {
                return 3;  // Forfeit
            } else if (selectedButton == GameButton::QUIT) {
                return 4;  // Quit
            }
        }
    }
    
    return 0;  // Continue
}

void GameScreen::startRound(int round, const std::string& word) {
    currentRound = round;
    secretWord = word;
    
    // Initialize exposed pattern
    exposedPattern = "";
    for (char c : word) {
        if (std::isalpha(c)) {
            exposedPattern += "_";
        } else {
            exposedPattern += c;  // Keep spaces, hyphens, etc.
        }
    }
    
    guessedLetters.clear();
    remainingAttempts = 7;
    hangmanStage = 0;
    inputBuffer = "";
    statusMessage = "Round " + std::to_string(round) + " starts!";
}

void GameScreen::setPlayers(const PlayerScore& p1, const PlayerScore& p2) {
    player1 = p1;
    player2 = p2;
}

void GameScreen::setCurrentUser(const std::string& username) {
    currentUser = username;
}

void GameScreen::setMyTurn(bool isTurn) {
    isMyTurn = isTurn;
}

void GameScreen::updateExposedPattern(const std::string& pattern) {
    exposedPattern = pattern;
}

void GameScreen::updateScores(int p1Score, int p2Score) {
    player1.score = p1Score;
    player2.score = p2Score;
}

void GameScreen::decrementAttempts() {
    if (remainingAttempts > 0) {
        remainingAttempts--;
    }
}

void GameScreen::incrementHangman() {
    if (hangmanStage < 7) {
        hangmanStage++;
    }
}

void GameScreen::reset() {
    currentRound = 1;
    secretWord = "";
    exposedPattern = "";
    guessedLetters.clear();
    recentGuesses.clear();
    player1 = PlayerScore();
    player2 = PlayerScore();
    remainingAttempts = 7;
    hangmanStage = 0;
    inputBuffer = "";
    statusMessage = "";
}