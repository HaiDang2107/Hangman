#include "ui/LoginScreen.h"
#include <cstring>

LoginScreen::LoginScreen() 
    : selectedOption(LoginOption::LOGIN),
      activeField(InputField::NONE),
      username(""),
      password(""),
      errorMessage("") {
    
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
}

LoginScreen::~LoginScreen() {
    if (mainWin) {
        delwin(mainWin);
    }
}

void LoginScreen::drawBorder() {
    box(mainWin, 0, 0);
}

void LoginScreen::drawTitle() {
    wattron(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    // ASCII Art Title
    int startY = 2;
    mvwprintw(mainWin, startY++, (width - 50) / 2, " _   _    _    _   _  ____ __  __    _    _   _ ");
    mvwprintw(mainWin, startY++, (width - 50) / 2, "| | | |  / \\  | \\ | |/ ___|  \\/  |  / \\  | \\ | |");
    mvwprintw(mainWin, startY++, (width - 50) / 2, "| |_| | / _ \\ |  \\| | |  _| |\\/| | / _ \\ |  \\| |");
    mvwprintw(mainWin, startY++, (width - 50) / 2, "|  _  |/ ___ \\| |\\  | |_| | |  | |/ ___ \\| |\\  |");
    mvwprintw(mainWin, startY++, (width - 50) / 2, "|_| |_/_/   \\_\\_| \\_|\\____|_|  |_/_/   \\_\\_| \\_|");
    wattroff(mainWin, COLOR_PAIR(1) | A_BOLD);
    
    // Subtitle
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, startY + 1, (width - 25) / 2, "~ Online Multiplayer ~");
    wattroff(mainWin, COLOR_PAIR(3));
}

void LoginScreen::drawInputFields() {
    int startY = 10;
    int startX = (width - INPUT_WIDTH) / 2;
    
    // Username field
    if (activeField == InputField::USERNAME) {
        wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(3));
    }
    mvwprintw(mainWin, startY, startX - 12, "Username:");
    mvwprintw(mainWin, startY, startX, "[%-28s]", username.c_str());
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    wattroff(mainWin, COLOR_PAIR(3));
    
    // Password field
    startY += 2;
    if (activeField == InputField::PASSWORD) {
        wattron(mainWin, COLOR_PAIR(5) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(3));
    }
    mvwprintw(mainWin, startY, startX - 12, "Password:");
    
    // Hiển thị password dạng ***
    std::string maskedPassword(password.length(), '*');
    mvwprintw(mainWin, startY, startX, "[%-28s]", maskedPassword.c_str());
    wattroff(mainWin, COLOR_PAIR(5) | A_BOLD);
    wattroff(mainWin, COLOR_PAIR(3));
}

void LoginScreen::drawMenu() {
    int startY = MENU_START_Y;
    int centerX = width / 2;
    
    wattron(mainWin, COLOR_PAIR(3));
    mvwprintw(mainWin, startY, centerX - 30, "Use UP/DOWN arrows to navigate | TAB to switch fields | Enter to submit");
    wattroff(mainWin, COLOR_PAIR(3));
    
    startY += 2;
    
    // Login option
    if (selectedOption == LoginOption::LOGIN) {
        wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(mainWin, startY, centerX - 10, ">  [  LOGIN  ]  <");
        wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(3));
        mvwprintw(mainWin, startY, centerX - 10, "   [  LOGIN  ]   ");
        wattroff(mainWin, COLOR_PAIR(3));
    }
    
    startY += 2;
    
    // Sign up option
    if (selectedOption == LoginOption::SIGNUP) {
        wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(mainWin, startY, centerX - 10, ">  [ SIGN UP ]  <");
        wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(3));
        mvwprintw(mainWin, startY, centerX - 10, "   [ SIGN UP ]   ");
        wattroff(mainWin, COLOR_PAIR(3));
    }
    
    startY += 2;
    
    // Exit option
    if (selectedOption == LoginOption::EXIT) {
        wattron(mainWin, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(mainWin, startY, centerX - 10, ">  [  EXIT   ]  <");
        wattroff(mainWin, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(mainWin, COLOR_PAIR(3));
        mvwprintw(mainWin, startY, centerX - 10, "   [  EXIT   ]   ");
        wattroff(mainWin, COLOR_PAIR(3));
    }
}

void LoginScreen::drawError() {
    if (!errorMessage.empty()) {
        wattron(mainWin, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(mainWin, height - 3, (width - errorMessage.length()) / 2, 
                  "%s", errorMessage.c_str());
        wattroff(mainWin, COLOR_PAIR(4) | A_BOLD);
    }
}

void LoginScreen::clearError() {
    errorMessage = "";
}

void LoginScreen::draw() {
    werase(mainWin);
    drawBorder();
    drawTitle();
    drawInputFields();
    drawMenu();
    drawError();
    wrefresh(mainWin);
}

void LoginScreen::handleMenuNavigation(int ch) {
    switch(ch) {
        case KEY_UP:
            if (selectedOption == LoginOption::SIGNUP) {
                selectedOption = LoginOption::LOGIN;
            } else if (selectedOption == LoginOption::EXIT) {
                selectedOption = LoginOption::SIGNUP;
            }
            clearError();
            break;
            
        case KEY_DOWN:
            if (selectedOption == LoginOption::LOGIN) {
                selectedOption = LoginOption::SIGNUP;
            } else if (selectedOption == LoginOption::SIGNUP) {
                selectedOption = LoginOption::EXIT;
            }
            clearError();
            break;
            
        case 9:  // Tab key - switch between input fields
            if (activeField == InputField::NONE) {
                switchToField(InputField::USERNAME);
            } else if (activeField == InputField::USERNAME) {
                switchToField(InputField::PASSWORD);
            } else {
                switchToField(InputField::USERNAME);
            }
            break;
            
        case 10:  // Enter key
        case KEY_ENTER:
            if (activeField == InputField::NONE) {
                // User pressed enter on menu option
                // Will be handled in main handleInput
            } else {
                // User finished typing, return to menu
                activeField = InputField::NONE;
            }
            break;
    }
}

void LoginScreen::handleFieldInput(int ch) {
    std::string* currentField = nullptr;
    
    if (activeField == InputField::USERNAME) {
        currentField = &username;
    } else if (activeField == InputField::PASSWORD) {
        currentField = &password;
    } else {
        return;
    }
    
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (!currentField->empty()) {
            currentField->pop_back();
        }
    } else if (ch == 27) {  // ESC key
        activeField = InputField::NONE;
    } else if (ch == 9) {  // Tab
        if (activeField == InputField::USERNAME) {
            switchToField(InputField::PASSWORD);
        } else {
            switchToField(InputField::USERNAME);
        }
    } else if (ch == 10 || ch == KEY_ENTER) {
        activeField = InputField::NONE;
    } else if (ch >= 32 && ch <= 126 && currentField->length() < 28) {
        // Printable characters
        *currentField += static_cast<char>(ch);
    }
    
    clearError();
}

void LoginScreen::switchToField(InputField field) {
    activeField = field;
    clearError();
}

bool LoginScreen::validateInput() {
    if (username.empty()) {
        setError("Username cannot be empty!");
        return false;
    }
    
    if (password.empty()) {
        setError("Password cannot be empty!");
        return false;
    }
    
    if (username.length() < 3) {
        setError("Username must be at least 3 characters!");
        return false;
    }
    
    if (password.length() < 4) {
        setError("Password must be at least 4 characters!");
        return false;
    }
    
    return true;
}

int LoginScreen::handleInput() {
    int ch = wgetch(mainWin);
    
    if (activeField != InputField::NONE) {
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
        if (selectedOption == LoginOption::EXIT) {
            return -1;  // Exit game
        }
        
        // SIGN UP không cần nhập gì, chuyển trực tiếp sang màn hình đăng ký
        if (selectedOption == LoginOption::SIGNUP) {
            return 2;  // Signup
        }
        
        // LOGIN yêu cầu nhập username và password
        if (selectedOption == LoginOption::LOGIN) {
            // Nếu chưa nhập gì, chuyển sang chế độ nhập username
            if (username.empty() && password.empty()) {
                switchToField(InputField::USERNAME);
                setError("Please enter your username and password (use TAB to switch fields)");
                return 0;
            }
            
            if (!validateInput()) {
                return 0;  // Validation failed, stay on screen
            }
            
            return 1;  // Login
        }
    }
    
    // Click on input fields - using 'u' for username, 'p' for password
    if (ch == 'u' || ch == 'U') {
        switchToField(InputField::USERNAME);
    } else if (ch == 'p' || ch == 'P') {
        switchToField(InputField::PASSWORD);
    }
    
    return 0;  // Continue
}

void LoginScreen::setError(const std::string& msg) {
    errorMessage = msg;
}

void LoginScreen::reset() {
    username = "";
    password = "";
    errorMessage = "";
    selectedOption = LoginOption::LOGIN;
    activeField = InputField::NONE;
}