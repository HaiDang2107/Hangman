#ifndef DRAWREQUESTDIALOG_H
#define DRAWREQUESTDIALOG_H

#include <string>

namespace hangman {

class DrawRequestDialog {
private:
    std::string fromUsername;
    
public:
    DrawRequestDialog(const std::string& fromUsername);
    
    // Returns: true = accept, false = decline
    bool show();
};

} // namespace hangman

#endif
