#include <ncurses.h>
#include "ui/LoginScreen.h"
#include "ui/SignUpScreen.h"
#include "ui/MainMenuScreen.h"
#include "ui/RoomScreen.h"
#include "ui/CreateRoomDialog.h"
#include "ui/OnlineUsersDialog.h"
#include "ui/InviteDialog.h"
#include "ui/PlayScreen.h"
#include "ui/DrawRequestDialog.h"
#include "ui/GameSummaryScreen.h"
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

struct GameStartNotification {
    uint32_t roomId;
    std::string opponentUsername;
    uint32_t wordLength;
};

struct GuessCharResultNotification {
    bool correct;
    std::string exposedPattern;
    uint8_t remainingAttempts;
    uint32_t scoreGained;
    uint32_t totalScore;
    uint8_t currentRound;
    bool isOpponentGuess;  // true if this is opponent's guess result
    bool isMyTurn;  // Whether it's my turn after this guess
};

struct GuessWordResultNotification {
    bool correct;
    std::string message;
    uint8_t remainingAttempts;
    uint32_t scoreGained;
    uint32_t totalScore;
    uint8_t currentRound;
    bool roundComplete;
    std::string nextWordPattern;
    bool isOpponentGuess;
    bool isMyTurn;  // Whether it's my turn after this guess
};

struct DrawRequestNotification {
    std::string fromUsername;
    uint32_t matchId;
};

struct GameEndNotification {
    uint32_t matchId;
    uint8_t resultCode;
    std::string summary;
};

struct GameSummaryNotification {
    S2C_GameSummary data;
};

std::queue<PendingInvite> g_pendingInvites;
std::queue<InviteResponseNotification> g_inviteResponses;
std::queue<PlayerReadyNotification> g_playerReadyUpdates;
std::queue<GameStartNotification> g_gameStartNotifications;
std::queue<GuessCharResultNotification> g_guessCharResults;
std::queue<GuessWordResultNotification> g_guessWordResults;
std::queue<DrawRequestNotification> g_drawRequests;
std::queue<GameEndNotification> g_gameEndNotifications;
std::queue<GameSummaryNotification> g_gameSummaries;
std::mutex g_notificationMutex;

enum class AppScreen {
    LOGIN,
    SIGNUP,
    MAIN_MENU,
    CREATE_ROOM,
    MATCH_HISTORY,
    RANKINGS,
    PLAY,
    EXIT
};

// Global user data
struct UserData {
    std::string username;
    int level;
    int wins;
    int losses;
};

// Global game session data (for transitioning to play screen)
struct GameSession {
    std::string roomName;
    std::string hostUsername;
    std::string guestUsername;
    uint32_t roomId;
    uint32_t matchId;
    uint32_t wordLength;
    bool isHost;
} g_gameSession;

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
    
    client.setGameStartHandler([](const S2C_GameStart& gameStart) {
        std::lock_guard<std::mutex> lock(g_notificationMutex);
        g_gameStartNotifications.push({
            gameStart.room_id,
            gameStart.opponent_username,
            gameStart.word_length
        });
    });
    
    // Gameplay notification handlers
    client.setGuessCharResultHandler([](const S2C_GuessCharResult& result) {
        std::lock_guard<std::mutex> lock(g_notificationMutex);
        g_guessCharResults.push({
            result.correct,
            result.exposed_pattern,
            result.remaining_attempts,
            result.score_gained,
            result.total_score,
            result.current_round,
            true,  // This is opponent's guess
            result.is_my_turn  // Whether it's my turn now
        });
    });
    
    client.setGuessWordResultHandler([](const S2C_GuessWordResult& result) {
        std::lock_guard<std::mutex> lock(g_notificationMutex);
        g_guessWordResults.push({
            result.correct,
            result.message,
            result.remaining_attempts,
            result.score_gained,
            result.total_score,
            result.current_round,
            result.round_complete,
            result.next_word_pattern,
            true,  // This is opponent's guess
            result.is_my_turn  // Whether it's my turn now
        });
    });
    
    client.setDrawRequestHandler([](const S2C_DrawRequest& request) {
        std::lock_guard<std::mutex> lock(g_notificationMutex);
        g_drawRequests.push({
            request.from_username,
            request.match_id
        });
    });
    
    client.setGameEndHandler([](const S2C_GameEnd& gameEnd) {
        std::lock_guard<std::mutex> lock(g_notificationMutex);
        g_gameEndNotifications.push({
            gameEnd.match_id,
            gameEnd.result_code,
            gameEnd.summary
        });
    });
    
    client.setGameSummaryHandler([](const S2C_GameSummary& summary) {
        std::lock_guard<std::mutex> lock(g_notificationMutex);
        g_gameSummaries.push({summary});
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
                PendingInvite invite;
                bool hasInvite = false;
                {
                    std::lock_guard<std::mutex> lock(g_notificationMutex);
                    if (!g_pendingInvites.empty()) {
                        invite = g_pendingInvites.front();
                        g_pendingInvites.pop();
                        hasInvite = true;
                    }
                }
                
                if (hasInvite) {
                    // Process invite OUTSIDE of mutex lock
                    // std::cerr << "[DEBUG] Showing invite dialog..." << std::endl;
                    // Show invite dialog
                    InviteDialog inviteDialog(invite.fromUsername, invite.roomId);
                    int inviteResult = inviteDialog.show();
                        // std::cerr << "[DEBUG] Invite dialog returned: " << inviteResult << std::endl;
                        
                        // CRITICAL: Reset ncurses state after dialog
                        cbreak();       // Disable line buffering
                        noecho();       // Don't echo typed characters
                        curs_set(0);    // Hide cursor
                        flushinp();     // Clear input buffer
                        // std::cerr << "[DEBUG] Terminal mode reset after dialog" << std::endl;
                        
                        if (inviteResult == 1) {
                            // Accepted - send response and wait for result
                            // std::cerr << "[DEBUG] Guest accepted invite from: " << invite.fromUsername << std::endl;
                            
                            // This will block until server responds with S2C_JoinRoomResult
                            // std::cerr << "[DEBUG] Calling respondInvite..." << std::endl;
                            auto joinResult = client.respondInvite(invite.fromUsername, true);
                            // std::cerr << "[DEBUG] respondInvite returned, code: " << static_cast<int>(joinResult.code) << std::endl;
                            
                            if (joinResult.code != ResultCode::SUCCESS) {
                                showMessage("Error", "Failed to join room: " + joinResult.message, 6);
                                napms(2000);
                                continue;
                            }
                            
                            // Event loop is already stopped by respondInvite(), no need to stop again
                            // std::cerr << "[DEBUG] Event loop should be STOPPED now" << std::endl;
                            // std::cerr << "[DEBUG] Join successful! Preparing for room screen..." << std::endl;
                            
                            // Clear screen but DON'T call endwin() - it destroys ncurses state
                            clear();
                            refresh();
                            flushinp(); // Clear input buffer
                            
                            // Successfully joined - navigate to room screen as guest
                            // std::cerr << "[DEBUG] Creating guest room screen..." << std::endl;
                            // Constructor: RoomScreen(roomName, hostName, currentUserName, isHost)
                            RoomScreen guestRoomScreen(invite.roomName, invite.fromUsername, currentUser.username, false);
                            guestRoomScreen.setGuestPlayer(currentUser.username);
                            
                            // std::cerr << "[DEBUG] Setting up guest room window..." << std::endl;
                            // CRITICAL: Explicitly set input mode on the room window
                            WINDOW* roomWin = guestRoomScreen.getMainWindow();
                            keypad(roomWin, TRUE);
                            wtimeout(roomWin, 50);
                            nodelay(roomWin, TRUE);  // Non-blocking mode
                            
                            // Make sure this window gets input focus
                            touchwin(roomWin);
                            wrefresh(roomWin);
                            
                            // Draw initial screen ONCE
                            // std::cerr << "[DEBUG] Drawing initial guest room screen..." << std::endl;
                            try {
                                guestRoomScreen.draw();
                                // std::cerr << "[DEBUG] draw() completed successfully" << std::endl;
                                wrefresh(guestRoomScreen.getMainWindow()); // Force refresh
                                // std::cerr << "[DEBUG] wrefresh() completed successfully" << std::endl;
                            } catch (const std::exception& e) {
                                std::cerr << "[ERROR] Exception during draw/wrefresh: " << e.what() << std::endl;
                                break;
                            }
                            
                            // std::cerr << "[DEBUG] Entering guest room loop..." << std::endl;
                            bool inGuestRoom = true;
                            int loopCount = 0;
                            while (inGuestRoom) {
                                loopCount++;
                                // Check for game start notification
                                {
                                    std::unique_lock<std::mutex> lock(g_notificationMutex, std::try_to_lock);
                                    if (!lock.owns_lock()) {
                                        // Continue without checking, will check next iteration
                                    } else {
                                        if (!g_gameStartNotifications.empty()) {
                                            auto gameStartInfo = g_gameStartNotifications.front();
                                            g_gameStartNotifications.pop();
                                        
                                            // Game started! Transition to play screen
                                            showMessage("Game Started!", "Host has started the game!", 2);
                                            napms(1500);
                                            
                                            // Store game session info
                                            g_gameSession.roomName = invite.roomName;
                                            g_gameSession.hostUsername = invite.fromUsername;
                                            g_gameSession.guestUsername = currentUser.username;
                                            g_gameSession.roomId = gameStartInfo.roomId;
                                            g_gameSession.matchId = gameStartInfo.roomId;  // matchId = roomId
                                            g_gameSession.wordLength = gameStartInfo.wordLength;
                                            g_gameSession.isHost = false;
                                            
                                            inGuestRoom = false;
                                            currentScreen = AppScreen::PLAY;
                                            break; // Exit loop immediately
                                        }
                                    }
                                }
                                // std::cerr << "[DEBUG] No game start notification" << std::endl;
                                
                                // Draw current state before waiting for input
                                // std::cerr << "[DEBUG] About to call draw()..." << std::endl;
                                guestRoomScreen.draw();
                                // std::cerr << "[DEBUG] draw() completed" << std::endl;
                                // std::cerr << "[DEBUG] draw() completed" << std::endl;
                                
                                // Ensure window is properly refreshed
                                // std::cerr << "[DEBUG] Refreshing window..." << std::endl;
                                touchwin(guestRoomScreen.getMainWindow());
                                wrefresh(guestRoomScreen.getMainWindow());
                                // std::cerr << "[DEBUG] Window refreshed" << std::endl;
                                
                                if (loopCount == 1) {
                                    // std::cerr << "[DEBUG] First iteration - checking window state..." << std::endl;
                                    // std::cerr << "[DEBUG] mainWin pointer: " << guestRoomScreen.getMainWindow() << std::endl;
                                }
                                
                                // Wait for input (with a 50ms timeout)
                                // std::cerr << "[DEBUG] Calling handleInput()..." << std::endl;
                                int roomResult = guestRoomScreen.handleInput();
                                // std::cerr << "[DEBUG] handleInput() returned: " << roomResult << std::endl;
                                
                                if (roomResult != 0 && loopCount % 10 == 0) {
                                    // std::cerr << "[DEBUG] Guest room handleInput returned: " << roomResult << std::endl;
                                }

                                // If a resize happened, re-apply timeout and continue
                                if (roomResult == 3) {
                                    wtimeout(guestRoomScreen.getMainWindow(), 50);
                                    continue;
                                }
                                
                                // If no input (timeout), just continue without extra draw
                                if (roomResult == 0) {
                                    continue;
                                }
                                
                                switch(roomResult) {
                                    case 1:  // READY (guest only)
                                        {
                                            // std::cerr << "[DEBUG] Guest pressed READY button" << std::endl;
                                            // Send ready status to server
                                            auto readyResponse = client.setReady(invite.roomId, true);
                                            // std::cerr << "[DEBUG] setReady returned, code: " << static_cast<int>(readyResponse.code) << std::endl;
                                            
                                            if (readyResponse.code == ResultCode::SUCCESS) {
                                                guestRoomScreen.setGuestReady(true);
                                                showMessage("Ready Status", "You are ready! Waiting for host to start...", 2);
                                                napms(1000);
                                                
                                                // Re-establish window state after message
                                                clear();
                                                refresh();
                                                wtimeout(guestRoomScreen.getMainWindow(), 50);
                                            } else {
                                                showMessage("Error", "Failed to set ready: " + readyResponse.message, 6);
                                                napms(1500);
                                                
                                                // Re-establish window state after message
                                                clear();
                                                refresh();
                                                wtimeout(guestRoomScreen.getMainWindow(), 50);
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
                                            
                                            // Re-establish window state after dialog/message
                                            clear();
                                            refresh();
                                            wtimeout(guestRoomScreen.getMainWindow(), 50);
                                            
                                            // Force redraw after dialog
                                            guestRoomScreen.draw();
                                        }
                                        break;
                                        
                                    case -1:  // Exit Room
                                        {
                                            // std::cerr << "[DEBUG] Guest exiting room..." << std::endl;
                                            auto leaveResponse = client.leaveRoom(invite.roomId);
                                            // std::cerr << "[DEBUG] leaveRoom returned, code: " << static_cast<int>(leaveResponse.code) << std::endl;
                                            if (leaveResponse.code != ResultCode::SUCCESS) {
                                                showMessage("Error", "Failed to leave room properly", 6);
                                                napms(1000);
                                                
                                                // Re-establish window state
                                                clear();
                                                refresh();
                                                wtimeout(guestRoomScreen.getMainWindow(), 50);
                                                
                                                guestRoomScreen.draw();
                                            }
                                            inGuestRoom = false;
                                        }
                                        break;
                                }
                            }
                            
                            // After leaving room, back to main menu (only if not transitioning to PLAY)
                            if (currentScreen != AppScreen::PLAY) {
                                currentScreen = AppScreen::MAIN_MENU;
                            }
                        } else {
                            // Declined - send response without waiting
                            client.respondInvite(invite.fromUsername, false);
                        }
                }
                
                // Draw menu (will be redrawn after input or on invite)
                mainMenuScreen.draw();
                int result = mainMenuScreen.handleInput();
                
                // If no input (timeout), continue loop to check notifications
                if (result == 0) {
                    continue;
                }
                
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
                std::string guestUsername = ""; // Track guest who joins
                
                // Set timeout for non-blocking input to check notifications
                wtimeout(roomScreen.getMainWindow(), 50);
                
                // Draw initial screen
                roomScreen.draw();
                
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
                                guestUsername = response.toUsername; // Save guest name
                                roomScreen.setGuestPlayer(response.toUsername);
                                showMessage("Player Joined", 
                                          response.toUsername + " has joined your room!", 2);
                                napms(1500);
                                
                                // Re-establish window state after message
                                clear();
                                refresh();
                                wtimeout(roomScreen.getMainWindow(), 50);
                                
                                // Force redraw after message closes
                                roomScreen.draw();
                            } else {
                                // Guest declined
                                showMessage("Invitation Declined", 
                                          response.toUsername + " declined your invitation.", 6);
                                napms(2000);
                                
                                // Re-establish window state after message
                                clear();
                                refresh();
                                wtimeout(roomScreen.getMainWindow(), 50);
                                
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
                    
                    // Handle input (with 50ms timeout)
                    int result = roomScreen.handleInput();
                    
                    // If a resize happened, re-apply timeout and continue
                    if (result == 3) {
                        wtimeout(roomScreen.getMainWindow(), 50);
                        continue;
                    }
                    
                    switch(result) {
                        case 1:  // START GAME (host only)
                            {
                                // Send start game request to server
                                showMessage("Starting Game...", "Please wait...", 2);
                                auto startResponse = client.startGame(roomId);
                                
                                if (startResponse.code == ResultCode::SUCCESS) {
                                    // Wait a moment for server to send S2C_GameStart notification
                                    napms(500);
                                    
                                    // Check for game start notification
                                    bool gameStartReceived = false;
                                    GameStartNotification gameStartInfo;
                                    {
                                        std::lock_guard<std::mutex> lock(g_notificationMutex);
                                        if (!g_gameStartNotifications.empty()) {
                                            gameStartInfo = g_gameStartNotifications.front();
                                            g_gameStartNotifications.pop();
                                            gameStartReceived = true;
                                        }
                                    }
                                    
                                    if (gameStartReceived) {
                                        // Enter game screen
                                        showMessage("Game Started!", "Starting match...", 2);
                                        napms(1000);
                                        
                                        // Store game session info
                                        g_gameSession.roomName = roomName;
                                        g_gameSession.hostUsername = currentUser.username;
                                        g_gameSession.guestUsername = gameStartInfo.opponentUsername;
                                        g_gameSession.roomId = roomId;
                                        g_gameSession.matchId = roomId;  // matchId = roomId
                                        g_gameSession.wordLength = gameStartInfo.wordLength;
                                        g_gameSession.isHost = true;
                                        
                                        // Exit room loop and transition to play screen
                                        inRoom = false;
                                        currentScreen = AppScreen::PLAY;
                                    } else {
                                        showMessage("Error", "Failed to receive game start notification", 6);
                                        napms(2000);
                                        
                                        // Re-establish window state
                                        clear();
                                        refresh();
                                        wtimeout(roomScreen.getMainWindow(), 50);
                                        
                                        roomScreen.draw();
                                    }
                                } else {
                                    showMessage("Error", "Failed to start game: " + startResponse.message, 6);
                                    napms(2000);
                                    
                                    // Re-establish window state
                                    clear();
                                    refresh();
                                    wtimeout(roomScreen.getMainWindow(), 50);
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
                                    
                                    // Re-establish window state
                                    clear();
                                    refresh();
                                    wtimeout(roomScreen.getMainWindow(), 50);
                                    
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
                                        
                                        // Re-establish window state
                                        clear();
                                        refresh();
                                        wtimeout(roomScreen.getMainWindow(), 50);
                                        
                                        // Force redraw after message
                                        roomScreen.draw();
                                    } else {
                                        // User cancelled - restore and redraw room screen
                                        clear();
                                        refresh();
                                        wtimeout(roomScreen.getMainWindow(), 50);
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
            
            case AppScreen::PLAY: {
                // Enter play screen with game session data
                PlayScreen playScreen(
                    g_gameSession.roomName,
                    g_gameSession.hostUsername,
                    g_gameSession.guestUsername,
                    currentUser.username,
                    g_gameSession.isHost,
                    g_gameSession.roomId,
                    g_gameSession.matchId,
                    g_gameSession.wordLength
                );
                
                bool inGame = true;
                while (inGame) {
                    // Always draw first to show current state
                    playScreen.draw();
                    
                    // Check for notifications with try_lock to avoid blocking
                    {
                        std::unique_lock<std::mutex> lock(g_notificationMutex, std::try_to_lock);
                        if (lock.owns_lock()) {
                            // Check guess char results (opponent's guess)
                            if (!g_guessCharResults.empty()) {
                                auto result = g_guessCharResults.front();
                                g_guessCharResults.pop();
                                
                                if (result.isOpponentGuess) {
                                    uint8_t oldRound = playScreen.getCurrentRound();
                                    
                                    playScreen.updateWordPattern(result.exposedPattern);
                                    playScreen.setRemainingAttempts(result.remainingAttempts);
                                    playScreen.setScore(result.totalScore);  // This is OUR score (we are the opponent)
                                    
                                    // Check if round changed
                                    if (result.currentRound > oldRound) {
                                        // Round transition happened - check if word was completed
                                        bool wordComplete = true;
                                        for (char c : result.exposedPattern) {
                                            if (c == '_') {
                                                wordComplete = false;
                                                break;
                                            }
                                        }
                                        
                                        if (wordComplete) {
                                            playScreen.setGameMessage("Opponent completed Round " + std::to_string((int)oldRound) + "! Moving to Round " + std::to_string((int)result.currentRound) + "...");
                                        } else {
                                            playScreen.setGameMessage("Round " + std::to_string((int)oldRound) + " over. Moving to Round " + std::to_string((int)result.currentRound) + "...");
                                        }
                                        
                                        // Draw once to show the message
                                        playScreen.draw();
                                        napms(2000);  // Pause 2 seconds
                                        
                                        // Now update round and transition
                                        playScreen.setRound(result.currentRound);
                                        playScreen.handleRoundTransition(result.exposedPattern);
                                        playScreen.setGameMessage("Round " + std::to_string((int)result.currentRound) + " started! Waiting for opponent...");
                                        
                                        // Opponent caused transition, so it's still their turn (they go first in new round)
                                        playScreen.setMyTurn(result.isMyTurn);
                                    } else {
                                        playScreen.setRound(result.currentRound);
                                        if (result.correct) {
                                            playScreen.setGameMessage("Opponent guessed correctly!");
                                        } else {
                                            playScreen.setGameMessage("Opponent guessed wrong!");
                                        }
                                        
                                        // Use server's turn information
                                        playScreen.setMyTurn(result.isMyTurn);
                                    }
                                    
                                    // Redraw immediately to show changes
                                    playScreen.draw();
                                }
                            }
                            
                            // Check guess word results (opponent's guess)
                            if (!g_guessWordResults.empty()) {
                                auto result = g_guessWordResults.front();
                                g_guessWordResults.pop();
                                
                                if (result.isOpponentGuess) {
                                    uint8_t oldRound = playScreen.getCurrentRound();
                                    playScreen.setScore(result.totalScore);
                                    
                                    if (result.roundComplete) {
                                        // Opponent completed a round - show message first
                                        playScreen.setGameMessage("Opponent completed Round " + std::to_string((int)oldRound) + "! Moving to Round " + std::to_string((int)result.currentRound) + "...");
                                        
                                        // Draw once to show the message
                                        playScreen.draw();
                                        napms(2000);  // Pause 2 seconds
                                        
                                        // Now transition
                                        playScreen.setRound(result.currentRound);
                                        playScreen.handleRoundTransition(result.nextWordPattern);
                                        playScreen.setGameMessage("Round " + std::to_string((int)result.currentRound) + " started! Waiting for opponent...");
                                        
                                        // Use server's turn information
                                        playScreen.setMyTurn(result.isMyTurn);
                                    } else {
                                        playScreen.setRound(result.currentRound);
                                        
                                        if (result.correct && result.currentRound == 3) {
                                            // Opponent won in round 3 - game over
                                            playScreen.setGameOver(false, "Opponent guessed the word! They scored " + std::to_string(result.totalScore));
                                        } else {
                                            playScreen.setRemainingAttempts(result.remainingAttempts);
                                            playScreen.setGameMessage(result.message);
                                            // Use server's turn information
                                            playScreen.setMyTurn(result.isMyTurn);
                                        }
                                    }
                                    
                                    // Redraw immediately to show changes
                                    playScreen.draw();
                                }
                            }
                            
                            // Check draw requests
                            if (!g_drawRequests.empty()) {
                                auto request = g_drawRequests.front();
                                g_drawRequests.pop();
                                
                                // Release lock before showing dialog
                                lock.unlock();
                                
                                DrawRequestDialog drawDialog(request.fromUsername);
                                bool accepted = drawDialog.show();
                                
                                if (accepted) {
                                    // Send accept - end game as draw (result_code=3)
                                    try {
                                        auto& client = GameClient::getInstance();
                                        client.endGame(g_gameSession.roomId, g_gameSession.matchId, 3, "Draw accepted");
                                        playScreen.setGameOver(true, "Game ended in a draw");
                                    } catch (const std::exception& e) {
                                        playScreen.setGameMessage(std::string("Error: ") + e.what());
                                    }
                                } else {
                                    playScreen.setGameMessage("Draw request declined");
                                }
                                
                                // Redraw after dialog
                                playScreen.draw();
                                
                                // Reacquire lock for next iteration
                                lock.lock();
                            }
                            
                            // Check game end notifications
                            if (!g_gameEndNotifications.empty()) {
                                auto gameEnd = g_gameEndNotifications.front();
                                g_gameEndNotifications.pop();
                                
                                // result_code: 0 = resignation, 1 = win, 2 = loss, 3 = draw
                                if (gameEnd.resultCode == 0) {
                                    playScreen.setGameOver(true, "Opponent resigned - You win!");
                                } else if (gameEnd.resultCode == 1) {
                                    playScreen.setGameOver(false, "You lost!");
                                } else if (gameEnd.resultCode == 2) {
                                    playScreen.setGameOver(true, "You won!");
                                } else if (gameEnd.resultCode == 3) {
                                    playScreen.setGameOver(true, "Game ended in a draw");
                                }
                                
                                // Redraw immediately to show changes
                                playScreen.draw();
                            }
                            
                            // Check game summary notifications
                            if (!g_gameSummaries.empty()) {
                                auto summary = g_gameSummaries.front();
                                g_gameSummaries.pop();
                                
                                // Release lock before showing summary screen
                                lock.unlock();
                                
                                // Show game summary
                                GameSummaryScreen summaryScreen;
                                summaryScreen.show(summary.data);
                                summaryScreen.run();
                                
                                // Return to main menu after summary
                                if (summaryScreen.shouldReturnToMenu()) {
                                    inGame = false;
                                    currentScreen = AppScreen::MAIN_MENU;
                                }
                                
                                // Reacquire lock for next iteration
                                lock.lock();
                            }
                        }
                    }
                    
                    int result = playScreen.handleInput();
                    
                    if (result == -1) {
                        // Exit game
                        showMessage("Game Ended", "Returning to main menu...", 2);
                        napms(1500);
                        inGame = false;
                        currentScreen = AppScreen::MAIN_MENU;
                    }
                }
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