#include "service/MatchService.h"
#include "service/AuthService.h"
#include "service/RoomService.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <filesystem>

namespace hangman {

static MatchService* g_matchService = nullptr;

MatchService& MatchService::getInstance() {
    if (!g_matchService) {
        g_matchService = new MatchService();
    }
    return *g_matchService;
}

void MatchService::startMatch(uint32_t roomId, const std::vector<std::string>& players, const std::string& word) {
    std::lock_guard<std::mutex> lock(matchesMutex);
    
    Match match;
    match.matchId = roomId; // Use roomId as matchId for simplicity
    match.roomId = roomId;
    match.word = word;
    match.active = true;

    for (const auto& p : players) {
        PlayerMatchState state;
        state.username = p;
        state.remainingAttempts = 6;
        match.playerStates[p] = state;
    }

    matches[roomId] = match;
    std::cout << "Match started for room " << roomId << " with word " << word << std::endl;
}

std::string MatchService::getExposedPattern(const std::string& word, const std::set<char>& guessed) {
    std::string pattern = "";
    for (char c : word) {
        if (guessed.count(c)) {
            pattern += c;
        } else {
            pattern += '_';
        }
        pattern += ' ';
    }
    if (!pattern.empty()) pattern.pop_back();
    return pattern;
}

std::string MatchService::getOpponentPattern(uint32_t roomId, const std::string& guesserUsername, 
                                             char guessedChar, bool wasCorrect) {
    std::lock_guard<std::mutex> lock(matchesMutex);
    auto it = matches.find(roomId);
    if (it == matches.end()) return "";
    
    Match& match = it->second;
    
    // Find opponent
    std::string opponentName;
    for (const auto& pair : match.playerStates) {
        if (pair.first != guesserUsername) {
            opponentName = pair.first;
            break;
        }
    }
    
    if (opponentName.empty()) return "";
    
    PlayerMatchState& opponentState = match.playerStates[opponentName];
    
    // If the guess was correct, add it to opponent's guessed chars too
    // so opponent can see the revealed letter
    if (wasCorrect) {
        opponentState.guessedChars.insert(guessedChar);
    }
    
    // Generate pattern based on opponent's guessed chars
    return getExposedPattern(match.word, opponentState.guessedChars);
}

GuessCharResult MatchService::guessChar(const C2S_GuessChar& request) {
    GuessCharResult result;
    result.success = false;
    result.opponentFd = -1;

    std::string username;
    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        result.errorPacket.message = "Invalid session";
        return result;
    }

    std::lock_guard<std::mutex> lock(matchesMutex);
    auto it = matches.find(request.room_id);
    if (it == matches.end() || !it->second.active) {
        result.errorPacket.message = "Match not found or ended";
        return result;
    }

    Match& match = it->second;
    if (match.playerStates.find(username) == match.playerStates.end()) {
        result.errorPacket.message = "Player not in match";
        return result;
    }

    PlayerMatchState& state = match.playerStates[username];
    if (state.finished) {
        result.errorPacket.message = "You already finished";
        return result;
    }

    // Process guess
    bool correct = false;
    if (match.word.find(request.ch) != std::string::npos) {
        correct = true;
    } else {
        if (state.remainingAttempts > 0) state.remainingAttempts--;
    }
    state.guessedChars.insert(request.ch);

    result.success = true;
    result.guesserUsername = username;
    result.resultPacket.correct = correct;
    result.resultPacket.remaining_attempts = state.remainingAttempts;
    result.resultPacket.exposed_pattern = getExposedPattern(match.word, state.guessedChars);

    // Find opponent
    for (const auto& pair : match.playerStates) {
        if (pair.first != username) {
            result.opponentFd = AuthService::getInstance().getClientFd(pair.first);
            break;
        }
    }

    // Check win/loss condition
    bool won = true;
    for (char c : match.word) {
        if (state.guessedChars.find(c) == state.guessedChars.end()) {
            won = false;
            break;
        }
    }

    if (won) {
        state.finished = true;
        state.won = true;
        // Note: Client should send EndGame or we can trigger it. 
        // But protocol has C2S_EndGame, so maybe client detects win and sends EndGame?
        // Or server sends GameEnd?
        // Usually server decides. But let's stick to protocol.
        // If client sees pattern complete, it knows it won.
    } else if (state.remainingAttempts == 0) {
        state.finished = true;
        state.won = false;
    }

    return result;
}

GuessWordResult MatchService::guessWord(const C2S_GuessWord& request) {
    GuessWordResult result;
    result.success = false;
    result.gameEnded = false;
    result.opponentFd = -1;

    std::string username;
    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        result.errorPacket.message = "Invalid session";
        return result;
    }

    std::lock_guard<std::mutex> lock(matchesMutex);
    auto it = matches.find(request.room_id);
    if (it == matches.end() || !it->second.active) {
        result.errorPacket.message = "Match not found";
        return result;
    }

    Match& match = it->second;
    PlayerMatchState& state = match.playerStates[username];
    
    if (state.finished) {
        result.errorPacket.message = "Already finished";
        return result;
    }

    bool correct = (request.word == match.word);
    if (!correct) {
        if (state.remainingAttempts > 0) state.remainingAttempts--;
    }

    result.success = true;
    result.guesserUsername = username;
    result.resultPacket.correct = correct;
    result.resultPacket.remaining_attempts = state.remainingAttempts;
    
    // Find opponent
    for (const auto& pair : match.playerStates) {
        if (pair.first != username) {
            result.opponentFd = AuthService::getInstance().getClientFd(pair.first);
            break;
        }
    }
    
    if (correct) {
        result.resultPacket.message = "Correct! You win!";
        state.finished = true;
        state.won = true;
        result.gameEnded = true;
    } else {
        result.resultPacket.message = "Incorrect!";
        if (state.remainingAttempts == 0) {
            state.finished = true;
            state.won = false;
            result.gameEnded = true;
        }
    }

    return result;
}

std::pair<int, S2C_DrawRequest> MatchService::requestDraw(const C2S_RequestDraw& request) {
    std::string username;
    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        return {-1, {}};
    }

    std::lock_guard<std::mutex> lock(matchesMutex);
    auto it = matches.find(request.room_id);
    if (it == matches.end()) return {-1, {}};

    Match& match = it->second;
    int opponentFd = -1;
    
    for (const auto& pair : match.playerStates) {
        if (pair.first != username) {
            opponentFd = AuthService::getInstance().getClientFd(pair.first);
            break;
        }
    }

    S2C_DrawRequest packet;
    packet.from_username = username;
    packet.match_id = request.match_id;

    return {opponentFd, packet};
}

EndGameResult MatchService::endGame(const C2S_EndGame& request) {
    EndGameResult result;
    result.success = false;
    result.opponentFd = -1;

    std::string username;
    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        result.errorPacket.message = "Invalid session";
        return result;
    }

    std::lock_guard<std::mutex> lock(matchesMutex);
    auto it = matches.find(request.room_id);
    if (it == matches.end()) {
        result.errorPacket.message = "Match not found";
        return result;
    }

    Match& match = it->second;
    
    // Find opponent
    std::string opponentName;
    for (const auto& pair : match.playerStates) {
        if (pair.first != username) {
            opponentName = pair.first;
            result.opponentFd = AuthService::getInstance().getClientFd(opponentName);
            break;
        }
    }

    // Update stats and history
    // result_code: 0 = resignation, 1 = win, 2 = loss, 3 = draw
    
    uint32_t points = 0;
    bool isWin = false;
    std::string summary = request.message;

    if (request.result_code == 1) { // Win
        points = 10;
        isWin = true;
    } else if (request.result_code == 3) { // Draw
        points = 1;
    }

    // Update this user
    AuthService::getInstance().updateUserStats(username, isWin, points);
    saveHistory(username, opponentName, request.result_code, summary);

    // If resignation, opponent wins
    if (request.result_code == 0) {
        AuthService::getInstance().updateUserStats(opponentName, true, 10);
        saveHistory(opponentName, username, 1, "Opponent resigned");
    } 
    // If draw, update opponent too (assuming both agreed)
    else if (request.result_code == 3) {
        AuthService::getInstance().updateUserStats(opponentName, false, 1);
        saveHistory(opponentName, username, 3, "Draw");
    }
    // If win/loss, opponent update might happen when they send EndGame?
    // Or we update both now?
    // Usually in P2P logic, each client sends EndGame.
    // But if we want to be secure, server should decide.
    // Here we trust client for now as per protocol structure.
    
    // Construct response
    result.success = true;
    result.endPacket.match_id = request.match_id;
    result.endPacket.result_code = request.result_code;
    result.endPacket.summary = "Game Over";

    // Clean up match if both finished?
    // For now keep it simple.
    
    return result;
}

void MatchService::saveHistory(const std::string& username, const std::string& opponent, uint8_t result, const std::string& summary) {
    std::string dir = "database/history/" + username;
    try {
        std::filesystem::create_directories(dir);
    } catch (...) {}

    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << dir << "/" << t << ".txt";
    
    std::ofstream file(ss.str());
    if (file.is_open()) {
        // Format: match_id:opponent:result:timestamp:summary
        file << "0" << ":" << opponent << ":" << (int)result << ":" << t << ":" << summary << "\n";
        file.close();
    }
}

} // namespace hangman
