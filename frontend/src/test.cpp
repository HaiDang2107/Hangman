#include <ncurses.h>
#include "ui/LoginScreen.h"
#include "ui/SignUpScreen.h"
#include "ui/MainMenuScreen.h"
#include "ui/RoomScreen.h"
#include "ui/CreateRoomDialog.h"
#include "ui/OnlineUsersDialog.h"
#include "ui/InviteDialog.h"
#include "network/GameClient.h"
#include <string>
#include <iostream>
#include <queue>
#include <mutex>

using namespace hangman;

// Global notification queue
struct PendingInvite {
    std::string fromUsername;
    uint32_t roomId;
    std::string roomName;
};

struct InviteResponseNotification {
    std::string toUsername;
    bool accepted;
    std::string message;
};

struct PlayerReadyNotification {
    std::string username;
    bool ready;
};

std::queue<PendingInvite> g_pendingInvites;
std::queue<InviteResponseNotification> g_inviteResponses;
std::queue<PlayerReadyNotification> g_playerReadyUpdates;
std::mutex g_notificationMutex;

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
    
    // Setup event handlers for notifications
    auto& client = GameClient::getInstance();
    
    client.setInviteReceivedHandler([](const S2C_InviteReceived& invite) {
        std::lock_guard<std::mutex> lock(g_notificationMutex);
        g_pendingInvites.push({invite.from_username, invite.room_id, invite.room_name});
    });
    
    client.setInviteResponseHandler([](const S2C_InviteResponse& response) {
        std::lock_guard<std::mutex> lock(g_notificationMutex);
        g_inviteResponses.push({response.to_username, response.accepted, response.message});
    });
    
    client.setPlayerReadyUpdateHandler([](const S2C_PlayerReadyUpdate& update) {
        std::lock_guard<std::mutex> lock(g_notificationMutex);
        g_playerReadyUpdates.push({update.username, update.ready});
    });
    
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
                // Check for pending invitations
                {
                    std::lock_guard<std::mutex> lock(g_notificationMutex);
                    if (!g_pendingInvites.empty()) {
                        auto invite = g_pendingInvites.front();
                        g_pendingInvites.pop();
                        
                        // Show invite dialog
                        InviteDialog inviteDialog(invite.fromUsername, invite.roomId);
                        int inviteResult = inviteDialog.show();
                        
                        if (inviteResult == 1) {
                            // Accepted - send response and wait for result
                            showMessage("Joining Room...", "Please wait...", 2);
                            
                            // This will block until server responds with S2C_JoinRoomResult
                            auto joinResult = client.respondInvite(invite.fromUsername, true);
                            
                            if (joinResult.code != ResultCode::SUCCESS) {
                                showMessage("Error", "Failed to join room: " + joinResult.message, 6);
                                napms(2000);
                                continue;
                            }
                            
                            // Clear the "Joining Room..." message
                            clear();
                            refresh();
                            
                            // Successfully joined - navigate to room screen as guest
                            // Constructor: RoomScreen(roomName, hostName, currentUserName, isHost)
                            RoomScreen guestRoomScreen(invite.roomName, invite.fromUsername, currentUser.username, false);
                            guestRoomScreen.setGuestPlayer(currentUser.username);
                            
                            // Set timeout for non-blocking input
                            wtimeout(guestRoomScreen.getMainWindow(), 50);
                            
                            // Draw initial screen
                            guestRoomScreen.draw();
                            
                            bool inGuestRoom = true;
                            while (inGuestRoom) {
                                // Check for game start notification
                                // Note: Event loop is stopped, so no need to lock mutex
                                // (No concurrent access possible)
                                
                                // Draw current state
                                guestRoomScreen.draw();
                                
                                // Wait for input (with a 50ms timeout)
                                int roomResult = guestRoomScreen.handleInput();

                                // If a resize happened, re-apply timeout and continue
                                if (roomResult == 3) {
                                    wtimeout(guestRoomScreen.getMainWindow(), 50);
                                    continue;
                                }
                                
                                switch(roomResult) {
                                    case 1:  // READY (guest only)
                                        {
                                            // Send ready status to server
                                            auto readyResponse = client.setReady(invite.roomId, true);
                                            
                                            if (readyResponse.code == ResultCode::SUCCESS) {
                                                showMessage("Ready Status", "You are ready! Waiting for host to start...", 2);
                                                napms(1000);
                                                guestRoomScreen.setGuestReady(true);
                                            } else {
                                                showMessage("Error", "Failed to set ready: " + readyResponse.message, 6);
                                                napms(1500);
                                            }
                                            // Force redraw after message
                                            guestRoomScreen.draw();
                                        }
                                        break;
                                    
                                    case 2:  // VIEW_FREE_USERS (guest can view but not invite)
                                        {
                                            showMessage("Loading...", "Fetching online users...", 5);
                                            
                                            // Request online list from server
                                            auto onlineList = client.requestOnlineList();
                                            
                                            if (onlineList.users.empty()) {
                                                showMessage("No Users", "No free users available at the moment", 5);
                                                napms(2000);
                                            } else {
                                                // Show online users dialog (view only for guest)
                                                OnlineUsersDialog usersDialog;
                                                usersDialog.setUserList(onlineList.users);
                                                usersDialog.show();
                                            }
                                            // Force redraw after dialog
                                            guestRoomScreen.draw();
                                        }
                                        break;
                                        
                                    case -1:  // Exit Room
                                        {
                                            auto leaveResponse = client.leaveRoom(invite.roomId);
                                            if (leaveResponse.code != ResultCode::SUCCESS) {
                                                showMessage("Error", "Failed to leave room properly", 6);
                                                napms(1000);
                                                guestRoomScreen.draw();
                                            }
                                            inGuestRoom = false;
                                        }
                                        break;
                                }
                            }
                            
                            // After leaving room, back to main menu
                            currentScreen = AppScreen::MAIN_MENU;
                        } else {
                            // Declined - send response without waiting
                            client.respondInvite(invite.fromUsername, false);
                        }
                    }
                }
                
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
                // Show dialog to enter room name
                CreateRoomDialog createDialog;
                int dialogResult = createDialog.show();
                
                if (dialogResult == -1) {
                    // User cancelled
                    currentScreen = AppScreen::MAIN_MENU;
                    break;
                }
                
                // User confirmed - create room on server
                std::string roomName = createDialog.getRoomName();
                showMessage("Creating Room...", "Please wait...", 5);
                
                auto& client = GameClient::getInstance();
                auto createResponse = client.createRoom(roomName);
                
                if (createResponse.code != ResultCode::SUCCESS) {
                    showMessage("Error", "Failed to create room: " + createResponse.message, 6);
                    napms(2000);
                    currentScreen = AppScreen::MAIN_MENU;
                    break;
                }
                
                // Initialize room screen with room ID from server
                uint32_t roomId = createResponse.room_id;
                RoomScreen roomScreen(roomName, currentUser.username, currentUser.username, true);
                
                bool inRoom = true;
                while (inRoom) {
                    // Check for invite responses and ready updates from event loop
                    {
                        std::lock_guard<std::mutex> lock(g_notificationMutex);
                        
                        // Handle invite responses
                        if (!g_inviteResponses.empty()) {
                            auto response = g_inviteResponses.front();
                            g_inviteResponses.pop();
                            
                            if (response.accepted) {
                                // Guest accepted - add them to room
                                roomScreen.setGuestPlayer(response.toUsername);
                                showMessage("Player Joined", 
                                          response.toUsername + " has joined your room!", 2);
                                napms(1500);
                                // Force redraw after message closes
                                roomScreen.draw();
                            } else {
                                // Guest declined
                                showMessage("Invitation Declined", 
                                          response.toUsername + " declined your invitation.", 6);
                                napms(2000);
                                // Force redraw after message closes
                                roomScreen.draw();
                            }
                        }
                        
                        // Handle guest ready updates
                        if (!g_playerReadyUpdates.empty()) {
                            auto update = g_playerReadyUpdates.front();
                            g_playerReadyUpdates.pop();
                            
                            // Update guest ready status (should match guest player name)
                            roomScreen.setGuestReady(update.ready);
                            // Force redraw to show updated status
                            roomScreen.draw();
                        }
                    }
                    
                    // Draw current state
                    roomScreen.draw();
                    
                    // Handle input
                    int result = roomScreen.handleInput();
                    
                    switch(result) {
                        case 1:  // START GAME (host only)
                            {
                                // Send start game request to server
                                showMessage("Starting Game...", "Please wait...", 2);
                                auto startResponse = client.startGame(roomId);
                                
                                if (startResponse.code == ResultCode::SUCCESS) {
                                    // TODO: Enter game screen
                                    showMessage("Game Started!", "Starting match...", 2);
                                    napms(2000);
                                    // For now, exit room
                                    inRoom = false;
                                    currentScreen = AppScreen::MAIN_MENU;
                                } else {
                                    showMessage("Error", "Failed to start game: " + startResponse.message, 6);
                                    napms(2000);
                                }
                                roomScreen.draw();
                            }
                            break;
                            
                        case 2:  // View Free Users
                            {
                                showMessage("Loading...", "Fetching online users...", 5);
                                
                                // Request online list from server
                                auto onlineList = client.requestOnlineList();
                                
                                if (onlineList.users.empty()) {
                                    showMessage("No Users", "No free users available at the moment", 5);
                                    napms(2000);
                                    // Force redraw after message
                                    roomScreen.draw();
                                } else {
                                    // Show online users dialog
                                    OnlineUsersDialog usersDialog;
                                    usersDialog.setUserList(onlineList.users);
                                    
                                    int dialogResult = usersDialog.show();
                                    
                                    if (dialogResult == 1) {
                                        // User wants to invite
                                        std::string selectedUser = usersDialog.getSelectedUser();
                                        
                                        // Send invite packet to server
                                        client.sendInvite(selectedUser, roomId);
                                        
                                        showMessage("Invite Sent", 
                                                   "Invitation sent to " + selectedUser + ". Waiting for response...", 2);
                                        napms(2000);
                                        // Force redraw after message
                                        roomScreen.draw();
                                    } else {
                                        // User cancelled - redraw room screen
                                        roomScreen.draw();
                                    }
                                }
                            }
                            break;
                            
                        case -1:  // Exit Room
                            {
                                // Leave room on server
                                auto leaveResponse = client.leaveRoom(roomId);
                                if (leaveResponse.code != ResultCode::SUCCESS) {
                                    showMessage("Error", "Failed to leave room properly", 6);
                                    napms(1000);
                                }
                                inRoom = false;
                                currentScreen = AppScreen::MAIN_MENU;
                            }
                            break;
                    }
                }
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