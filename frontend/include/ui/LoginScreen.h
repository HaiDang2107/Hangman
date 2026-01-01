#ifndef LOGIN_SCREEN_H
#define LOGIN_SCREEN_H

#include <ncurses.h>
#include <string>

enum class LoginOption {
    LOGIN,
    SIGNUP,
    EXIT
};

enum class InputField {
    USERNAME,
    PASSWORD,
    NONE
};

class LoginScreen {
private:
    WINDOW* mainWin;
    WINDOW* inputWin;
    int height, width;
    
    // State
    LoginOption selectedOption;
    InputField activeField;
    std::string username;
    std::string password;
    std::string errorMessage;
    
    // UI Constants
    static const int MENU_START_Y = 15;
    static const int INPUT_WIDTH = 30;
    static const int INPUT_HEIGHT = 3;
    
    // Private methods
    void drawBorder();
    void drawTitle();
    void drawInputFields();
    void drawMenu();
    void drawError();
    void clearError();
    
    void handleMenuNavigation(int ch);
    void handleFieldInput(int ch);
    void switchToField(InputField field);
    
    bool validateInput();
    
public:
    LoginScreen();
    ~LoginScreen();
    
    void draw();
    int handleInput();  // Returns: 0=continue, 1=login success, 2=signup, -1=exit
    
    std::string getUsername() const { return username; }
    std::string getPassword() const { return password; }
    void setError(const std::string& msg);
    void reset();
};

#endif // LOGIN_SCREEN_H