#ifndef PLAYSCREEN_H
#define PLAYSCREEN_H

#include <string>

namespace hangman {

class PlayScreen {
private:
    std::string roomName;
    std::string hostUsername;
    std::string guestUsername;
    std::string currentUsername;
    bool isHost;
    
public:
    PlayScreen(const std::string& roomName,
               const std::string& hostUsername,
               const std::string& guestUsername,
               const std::string& currentUsername,
               bool isHost);
    
    void draw();
    int handleInput();  // Returns: -1 = exit game
};

} // namespace hangman

#endif
