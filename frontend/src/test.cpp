#include <ncurses.h>
#include "ui/LoginScreen.h"
#include "ui/SignUpScreen.h"
#include "ui/MainMenuScreen.h"
#include "network/GameClient.h"
#include <string>

using namespace hangman;

enum class AppScreen {
    LOGIN,
    SIGNUP,
    MAIN_MENU,
    CREATE_ROOM,
    MATCH_HISTORY,
    RANKINGS,
    EXIT
};

// Global user data
struct UserData {
    std::string username;
    int level;
    int wins;
    int losses;
};

void initNcurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    if (has_colors()) {
        start_color();
        use_default_colors();
    }
}

void cleanupNcurses() {
    endwin();
}

void showMessage(const std::string& title, const std::string& message, int color = 2) {
    clear();
    attron(COLOR_PAIR(color) | A_BOLD);
    mvprintw(LINES/2 - 1, (COLS - title.length())/2, "%s", title.c_str());
    attroff(COLOR_PAIR(color) | A_BOLD);
    
    attron(COLOR_PAIR(3));
    mvprintw(LINES/2 + 1, (COLS - message.length())/2, "%s", message.c_str());
    attroff(COLOR_PAIR(3));
    
    refresh();
}

bool realLogin(const std::string& username, const std::string& password, UserData& userData) {
    auto& client = GameClient::getInstance();
    
    if (!client.isConnected()) {
        if (!client.connect("127.0.0.1", 5000)) {
            return false;
        }
    }
    
    auto response = client.login(username, password);
    
    if (response.code == ResultCode::SUCCESS) {
        userData.username = username;
        userData.level = 0; // Not used yet
        userData.wins = response.num_of_wins;
        userData.losses = 0; // Calculate from total games - wins if needed
        return true;
    }
    
    return false;
}

bool realSignUp(const std::string& username, const std::string& password) {
    auto& client = GameClient::getInstance();
    
    if (!client.isConnected()) {
        if (!client.connect("127.0.0.1", 5000)) {
            return false;
        }
    }
    
    auto response = client.registerUser(username, password);
    return response.code == ResultCode::SUCCESS;
}

void showComingSoon(const std::string& feature) {
    clear();
    
    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(LINES/2 - 2, (COLS - 30)/2, "╔════════════════════════════╗");
    mvprintw(LINES/2 - 1, (COLS - 30)/2, "║   COMING SOON!             ║");
    mvprintw(LINES/2,     (COLS - 30)/2, "╚════════════════════════════╝");
    attroff(COLOR_PAIR(4) | A_BOLD);
    
    attron(COLOR_PAIR(3));
    std::string msg = feature + " feature will be implemented next!";
    mvprintw(LINES/2 + 2, (COLS - msg.length())/2, "%s", msg.c_str());
    mvprintw(LINES/2 + 4, (COLS - 30)/2, "Press any key to go back...");
    attroff(COLOR_PAIR(3));
    
    refresh();
    getch();
}

int main() {
    initNcurses();
    
    LoginScreen loginScreen;
    SignUpScreen signupScreen;
    MainMenuScreen mainMenuScreen;
    
    AppScreen currentScreen = AppScreen::LOGIN;
    UserData currentUser;
    bool running = true;
    
    while (running) {
        switch(currentScreen) {
            case AppScreen::LOGIN: {
                loginScreen.draw();
                int result = loginScreen.handleInput();
                
                switch(result) {
                    case -1:  // Exit
                        currentScreen = AppScreen::EXIT;
                        break;
                        
                    case 1:   // Login
                        {
                            showMessage("Connecting...", "Please wait...", 5);
                            
                            std::string username = loginScreen.getUsername();
                            std::string password = loginScreen.getPassword();
                            
                            bool loginSuccess = realLogin(username, password, currentUser);
                            
                            if (loginSuccess) {
                                showMessage("Success!", 
                                           "Welcome back, " + username + "!", 2);
                                napms(1500);
                                
                                mainMenuScreen.setUserInfo(
                                    currentUser.username,
                                    currentUser.level,
                                    currentUser.wins,
                                    currentUser.losses
                                );
                                currentScreen = AppScreen::MAIN_MENU;
                            } else {
                                loginScreen.setError("Invalid username or password!");
                            }
                        }
                        break;
                        
                    case 2:   // Go to Sign Up
                        signupScreen.reset();
                        currentScreen = AppScreen::SIGNUP;
                        break;
                }
                break;
            }
            
            case AppScreen::SIGNUP: {
                signupScreen.draw();
                int result = signupScreen.handleInput();
                
                switch(result) {
                    case -1:  // Back to Login
                        loginScreen.reset();
                        currentScreen = AppScreen::LOGIN;
                        break;
                        
                    case 1:   // Sign Up
                        {
                            showMessage("Creating account...", "Please wait...", 5);
                            
                            std::string username = signupScreen.getUsername();
                            std::string password = signupScreen.getPassword();
                            
                            bool signupSuccess = realSignUp(username, password);
                            
                            if (signupSuccess) {
                                showMessage("Account Created!", 
                                           "You can now login with your credentials.", 2);
                                napms(2000);
                                
                                loginScreen.reset();
                                loginScreen.setError("Account created successfully! Please login.");
                                currentScreen = AppScreen::LOGIN;
                            } else {
                                signupScreen.setError("Username already exists! Try another one.");
                            }
                        }
                        break;
                }
                break;
            }
            
            case AppScreen::MAIN_MENU: {
                mainMenuScreen.draw();
                int result = mainMenuScreen.handleInput();
                
                switch(result) {
                    case 1:  // Create Room
                        currentScreen = AppScreen::CREATE_ROOM;
                        break;
                        
                    case 2:  // View History
                        currentScreen = AppScreen::MATCH_HISTORY;
                        break;
                        
                    case 3:  // View Rankings
                        currentScreen = AppScreen::RANKINGS;
                        break;
                        
                    case -1:  // Logout
                        {
                            clear();
                            attron(COLOR_PAIR(4));
                            mvprintw(LINES/2, (COLS - 20)/2, "Logging out...");
                            attroff(COLOR_PAIR(4));
                            refresh();
                            
                            // Logout from server
                            auto& client = GameClient::getInstance();
                            if (client.hasValidSession()) {
                                client.logout();
                            }
                            
                            napms(1000);
                            
                            loginScreen.reset();
                            currentScreen = AppScreen::LOGIN;
                        }
                        break;
                        
                    case -2:  // Quit
                        currentScreen = AppScreen::EXIT;
                        break;
                }
                break;
            }
            
            case AppScreen::CREATE_ROOM: {
                showComingSoon("Create Room");
                currentScreen = AppScreen::MAIN_MENU;
                break;
            }
            
            case AppScreen::MATCH_HISTORY: {
                showComingSoon("Match History");
                currentScreen = AppScreen::MAIN_MENU;
                break;
            }
            
            case AppScreen::RANKINGS: {
                showComingSoon("Rankings");
                currentScreen = AppScreen::MAIN_MENU;
                break;
            }
            
            case AppScreen::EXIT:
                running = false;
                break;
        }
    }
    
    // Màn hình tạm biệt
    clear();
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(LINES/2 - 3, (COLS - 35)/2, "╔═══════════════════════════════════╗");
    mvprintw(LINES/2 - 2, (COLS - 35)/2, "║                                   ║");
    mvprintw(LINES/2 - 1, (COLS - 35)/2, "║      THANKS FOR PLAYING!          ║");
    mvprintw(LINES/2,     (COLS - 35)/2, "║      HANGMAN ONLINE               ║");
    mvprintw(LINES/2 + 1, (COLS - 35)/2, "║                                   ║");
    mvprintw(LINES/2 + 2, (COLS - 35)/2, "╚═══════════════════════════════════╝");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    attron(COLOR_PAIR(3));
    mvprintw(LINES/2 + 4, (COLS - 25)/2, "See you next time! 👋");
    attroff(COLOR_PAIR(3));
    
    refresh();
    napms(2000);
    
    // Disconnect from server
    GameClient::getInstance().disconnect();
    
    cleanupNcurses();
    return 0;
}