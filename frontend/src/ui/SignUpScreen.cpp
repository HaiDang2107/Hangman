#include "ui/SignUpScreen.h"
#include <cstring>

SignUpScreen::SignUpScreen() 
    : selectedOption(SignUpOption::SIGNUP),
      activeField(SignUpField::NONE),
      username(""),
      password(""),
      confirmPassword(""),
      errorMessage(""),
      successMessage("") {
    
    // Lấy kích thước màn hình
    getmaxyx(stdscr, height, width);
    
    // Tạo main window
    mainWin = newwin(height, width, 0, 0);
    keypad(mainWin, TRUE);
    
    // Khởi tạo colors
    init_pair(1, COLOR_CYAN, COLOR_BLACK);    // Title
    init_pair(2, COLOR_GREEN, COLOR_BLACK);   // Selected
    init_pair(3, COLOR_WHITE, COLOR_BLACK);   // Normal
    init_pair(4, COLOR_RED, COLOR_BLACK);     // Error
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);  // Active field
    init_pair(6, COLOR_GREEN, COLOR_BLACK);   // Success message
}

SignUpScreen::~SignUpScreen() {
    if (mainWin) {
        delwin(mainWin);
    }
}

void SignUpScreen::drawBorder() {
    box(mainWin, 0, 0);
}

void SignUpScreen::drawTitle() {
    wattron(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    // ASCII Art Title
    int startY = 2;
    mvwprintw(mainWin, startY++, (width - 40) / 2, " ____ ___ ____ _   _   _   _ ____  ");
    mvwprintw(mainWin, startY++, (width - 40) / 2, "/ ___|_ _/ ___| \\ | | | | | |  _ \\ ");
    mvwprintw(mainWin, startY++, (width - 40) / 2, "\\___ \\| | |  _|  \\| | | | | | |_) |");
    mvwprintw(mainWin, startY++, (width - 40) / 2, " ___) | | |_| | |\\  | | |_| |  __/ ");
    mvwprintw(mainWin, startY++, (width - 40) / 2, "|____/___\\____|_| \\_|  \\___/|_|    ");
    
    wattroff(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    // Subtitle
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, startY + 1, (width - 30) / 2, "Create your Hangman account");
    wattroff(mainWin, COLOR_PAIR(3));
}

void SignUpScreen::drawInputFields() {
    int startY = 10;
    int startX = (width - INPUT_WIDTH) / 2;
    
    // Username field
    if (activeField == SignUpField::USERNAME) {
        wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(3));
    }
    mvwprintw(mainWin, startY, startX - 18, "Username:");
    mvwprintw(mainWin, startY, startX, "[%-28s]", username.c_str());
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    wattroff(mainWin, COLOR_PAIR(3));
    
    // Password field
    startY += 2;
    if (activeField == SignUpField::PASSWORD) {
        wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(3));
    }
    mvwprintw(mainWin, startY, startX - 18, "Password:");
    
    // Hiển thị password dạng ***
    std::string maskedPassword(password.length(), '*');
    mvwprintw(mainWin, startY, startX, "[%-28s]", maskedPassword.c_str());
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    wattroff(mainWin, COLOR_PAIR(3));
    
    // Confirm Password field
    startY += 2;
    if (activeField == SignUpField::CONFIRM_PASSWORD) {
        wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(3));
    }
    mvwprintw(mainWin, startY, startX - 18, "Confirm Password:");
    
    // Hiển thị confirm password dạng ***
    std::string maskedConfirm(confirmPassword.length(), '*');
    mvwprintw(mainWin, startY, startX, "[%-28s]", maskedConfirm.c_str());
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    wattroff(mainWin, COLOR_PAIR(3));
    
    // Hints
    startY += 2;
    wattron(mainWin, COLOR_PAIR(3) | A_DIM);
    mvwprintw(mainWin, startY, startX - 18, "Tip: Use Tab to switch between fields");
    wattroff(mainWin, COLOR_PAIR(3) | A_DIM);
}

void SignUpScreen::drawMenu() {
    int startY = MENU_START_Y;
    int centerX = width / 2;
    
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, startY, centerX - 15, "Use UP/DOWN arrows to navigate, Enter to select");
    wattroff(mainWin, COLOR_PAIR(3));
    
    startY += 2;
    
    // Sign Up option
    if (selectedOption == SignUpOption::SIGNUP) {
        wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(mainWin, startY, centerX - 12, ">  [ CREATE ACCOUNT ]  <");
        wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(3));
        mvwprintw(mainWin, startY, centerX - 12, "   [ CREATE ACCOUNT ]   ");
        wattroff(mainWin, COLOR_PAIR(3));
    }
    
    startY += 2;
    
    // Back to Login option
    if (selectedOption == SignUpOption::BACK_TO_LOGIN) {
        wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(mainWin, startY, centerX - 12, ">  [ BACK TO LOGIN  ]  <");
        wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(3));
        mvwprintw(mainWin, startY, centerX - 12, "   [ BACK TO LOGIN  ]   ");
        wattroff(mainWin, COLOR_PAIR(3));
    }
}

void SignUpScreen::drawMessages() {
    // Error message
    if (!errorMessage.empty()) {
        wattron(mainWin, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(mainWin, height - 4, (width - errorMessage.length()) / 2, 
                  "%s", errorMessage.c_str());
        wattroff(mainWin, COLOR_PAIR(4) | A_BOLD);
    }
    
    // Success message
    if (!successMessage.empty()) {
        wattron(mainWin, COLOR_PAIR(6) | A_BOLD);
        mvwprintw(mainWin, height - 4, (width - successMessage.length()) / 2, 
                  "%s", successMessage.c_str());
        wattroff(mainWin, COLOR_PAIR(6) | A_BOLD);
    }
}

void SignUpScreen::clearMessages() {
    errorMessage = "";
    successMessage = "";
}

void SignUpScreen::draw() {
    werase(mainWin);
    drawBorder();
    drawTitle();
    drawInputFields();
    drawMenu();
    drawMessages();
    wrefresh(mainWin);
}

void SignUpScreen::handleMenuNavigation(int ch) {
    switch(ch) {
        case KEY_UP:
            if (selectedOption == SignUpOption::BACK_TO_LOGIN) {
                selectedOption = SignUpOption::SIGNUP;
            }
            clearMessages();
            break;
            
        case KEY_DOWN:
            if (selectedOption == SignUpOption::SIGNUP) {
                selectedOption = SignUpOption::BACK_TO_LOGIN;
            }
            clearMessages();
            break;
            
        case 9:  // Tab key - switch between input fields
            if (activeField == SignUpField::NONE) {
                switchToField(SignUpField::USERNAME);
            } else if (activeField == SignUpField::USERNAME) {
                switchToField(SignUpField::PASSWORD);
            } else if (activeField == SignUpField::PASSWORD) {
                switchToField(SignUpField::CONFIRM_PASSWORD);
            } else {
                switchToField(SignUpField::USERNAME);
            }
            break;
            
        case 10:  // Enter key
        case KEY_ENTER:
            if (activeField == SignUpField::NONE) {
                // User pressed enter on menu option
                // Will be handled in main handleInput
            } else {
                // User finished typing, return to menu
                activeField = SignUpField::NONE;
            }
            break;
    }
}

void SignUpScreen::handleFieldInput(int ch) {
    std::string* currentField = nullptr;
    
    if (activeField == SignUpField::USERNAME) {
        currentField = &username;
    } else if (activeField == SignUpField::PASSWORD) {
        currentField = &password;
    } else if (activeField == SignUpField::CONFIRM_PASSWORD) {
        currentField = &confirmPassword;
    } else {
        return;
    }
    
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (!currentField->empty()) {
            currentField->pop_back();
        }
    } else if (ch == 27) {  // ESC key
        activeField = SignUpField::NONE;
    } else if (ch == 9) {  // Tab
        if (activeField == SignUpField::USERNAME) {
            switchToField(SignUpField::PASSWORD);
        } else if (activeField == SignUpField::PASSWORD) {
            switchToField(SignUpField::CONFIRM_PASSWORD);
        } else {
            switchToField(SignUpField::USERNAME);
        }
    } else if (ch == 10 || ch == KEY_ENTER) {
        // Enter moves to next field or exits if on last field
        if (activeField == SignUpField::USERNAME) {
            switchToField(SignUpField::PASSWORD);
        } else if (activeField == SignUpField::PASSWORD) {
            switchToField(SignUpField::CONFIRM_PASSWORD);
        } else {
            activeField = SignUpField::NONE;
        }
    } else if (ch >= 32 && ch <= 126 && currentField->length() < 28) {
        // Printable characters
        *currentField += static_cast<char>(ch);
    }
    
    clearMessages();
}

void SignUpScreen::switchToField(SignUpField field) {
    activeField = field;
    clearMessages();
}

bool SignUpScreen::validateInput() {
    if (username.empty()) {
        setError("Username cannot be empty!");
        return false;
    }
    
    if (password.empty()) {
        setError("Password cannot be empty!");
        return false;
    }
    
    if (confirmPassword.empty()) {
        setError("Please confirm your password!");
        return false;
    }
    
    if (username.length() < 3) {
        setError("Username must be at least 3 characters!");
        return false;
    }
    
    if (username.length() > 20) {
        setError("Username must be at most 20 characters!");
        return false;
    }
    
    // Check username chỉ chứa chữ cái, số và underscore
    for (char c : username) {
        if (!isalnum(c) && c != '_') {
            setError("Username can only contain letters, numbers and underscore!");
            return false;
        }
    }
    
    if (password.length() < 6) {
        setError("Password must be at least 6 characters!");
        return false;
    }
    
    if (password.length() > 30) {
        setError("Password must be at most 30 characters!");
        return false;
    }
    
    if (password != confirmPassword) {
        setError("Passwords do not match!");
        return false;
    }
    
    return true;
}

int SignUpScreen::handleInput() {
    int ch = wgetch(mainWin);
    
    if (activeField != SignUpField::NONE) {
        // Currently editing a field
        handleFieldInput(ch);
        return 0;
    }
    
    // Handle menu navigation
    if (ch == KEY_UP || ch == KEY_DOWN || ch == 9) {
        handleMenuNavigation(ch);
        return 0;
    }
    
    // Handle Enter on menu options
    if (ch == 10 || ch == KEY_ENTER) {
        if (selectedOption == SignUpOption::BACK_TO_LOGIN) {
            return -1;  // Back to login
        }
        
        if (selectedOption == SignUpOption::SIGNUP) {
            if (!validateInput()) {
                return 0;  // Validation failed, stay on screen
            }
            return 1;  // Sign up
        }
    }
    
    // Quick shortcuts - 'u' for username, 'p' for password, 'c' for confirm
    if (ch == 'u' || ch == 'U') {
        switchToField(SignUpField::USERNAME);
    } else if (ch == 'p' || ch == 'P') {
        switchToField(SignUpField::PASSWORD);
    } else if (ch == 'c' || ch == 'C') {
        switchToField(SignUpField::CONFIRM_PASSWORD);
    }
    
    return 0;  // Continue
}

void SignUpScreen::setError(const std::string& msg) {
    errorMessage = msg;
    successMessage = "";
}

void SignUpScreen::setSuccess(const std::string& msg) {
    successMessage = msg;
    errorMessage = "";
}

void SignUpScreen::reset() {
    username = "";
    password = "";
    confirmPassword = "";
    errorMessage = "";
    successMessage = "";
    selectedOption = SignUpOption::SIGNUP;
    activeField = SignUpField::NONE;
}