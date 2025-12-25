#ifndef SIGNUP_SCREEN_H
#define SIGNUP_SCREEN_H

#include <ncurses.h>
#include <string>

enum class SignUpOption {
    SIGNUP,
    BACK_TO_LOGIN
};

enum class SignUpField {
    USERNAME,
    PASSWORD,
    CONFIRM_PASSWORD,
    NONE
};

class SignUpScreen {
private:
    WINDOW* mainWin;
    int height, width;
    
    // State
    SignUpOption selectedOption;
    SignUpField activeField;
    std::string username;
    std::string password;
    std::string confirmPassword;
    std::string errorMessage;
    std::string successMessage;
    
    // UI Constants
    static const int MENU_START_Y = 18;
    static const int INPUT_WIDTH = 30;
    
    // Private methods
    void drawBorder();
    void drawTitle();
    void drawInputFields();
    void drawMenu();
    void drawMessages();
    void clearMessages();
    
    void handleMenuNavigation(int ch);
    void handleFieldInput(int ch);
    void switchToField(SignUpField field);
    
    bool validateInput();
    
public:
    SignUpScreen();
    ~SignUpScreen();
    
    void draw();
    int handleInput();  // Returns: 0=continue, 1=signup success, -1=back to login
    
    std::string getUsername() const { return username; }
    std::string getPassword() const { return password; }
    void setError(const std::string& msg);
    void setSuccess(const std::string& msg);
    void reset();
};

#endif // SIGNUP_SCREEN_H