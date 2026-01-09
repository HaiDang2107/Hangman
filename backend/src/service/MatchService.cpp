#include "service/MatchService.h"
#include "service/AuthService.h"
#include "service/RoomService.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <filesystem>
#include <random>
#include <algorithm>
#include <sys/socket.h>

namespace hangman {

static MatchService* g_matchService = nullptr;

MatchService& MatchService::getInstance() {
    if (!g_matchService) {
        g_matchService = new MatchService();
        g_matchService->loadWords();  // Load words on first access
    }
    return *g_matchService;
}

void MatchService::loadWords() {
    // Load Round 1 words (4-7 letters)
    std::ifstream file1("database/words_round1.txt");
    if (file1.is_open()) {
        std::string word;
        while (std::getline(file1, word)) {
            // Remove whitespace and convert to uppercase
            word.erase(std::remove_if(word.begin(), word.end(), ::isspace), word.end());
            std::transform(word.begin(), word.end(), word.begin(), ::toupper);
            if (word.length() >= 4 && word.length() <= 7) {
                round1Words.push_back(word);
            }
        }
        file1.close();
    }
    
    // Load Round 2 words (8-12 letters)
    std::ifstream file2("database/words_round2.txt");
    if (file2.is_open()) {
        std::string word;
        while (std::getline(file2, word)) {
            word.erase(std::remove_if(word.begin(), word.end(), ::isspace), word.end());
            std::transform(word.begin(), word.end(), word.begin(), ::toupper);
            if (word.length() >= 8 && word.length() <= 12) {
                round2Words.push_back(word);
            }
        }
        file2.close();
    }
    
    // Load Round 3 words (10-15 letters)
    std::ifstream file3("database/words_round3.txt");
    if (file3.is_open()) {
        std::string word;
        while (std::getline(file3, word)) {
            word.erase(std::remove_if(word.begin(), word.end(), ::isspace), word.end());
            std::transform(word.begin(), word.end(), word.begin(), ::toupper);
            if (word.length() >= 10 && word.length() <= 15) {
                round3Words.push_back(word);
            }
        }
        file3.close();
    }
    
    std::cout << "Loaded " << round1Words.size() << " round 1 words, " 
              << round2Words.size() << " round 2 words, and "
              << round3Words.size() << " round 3 words" << std::endl;
}

std::string MatchService::getRandomWord(uint8_t round) {
    // For testing: always return first word
    if (round == 1 && !round1Words.empty()) {
        return round1Words[0];  // First word for testing
    } else if (round == 2 && !round2Words.empty()) {
        return round2Words[0];  // First word for testing
    } else if (round == 3 && !round3Words.empty()) {
        return round3Words[0];  // First word for testing
    }
    
    // Fallback
    if (round == 1) return "GAME";
    if (round == 2) return "COMPUTER";
    return "PROGRAMMING";
}

uint32_t MatchService::calculateScore(uint8_t round, bool correctGuess, bool isWordGuess, 
                                      char ch, const std::string& word) {
    if (isWordGuess) {
        // Guess entire word
        if (correctGuess) {
            // Bonus for guessing word: R1: +30, R2: +50, R3: +80
            if (round == 1) return 30;
            if (round == 2) return 50;
            return 80;
        } else {
            return 0;  // Will be handled as negative later
        }
    } else {
        // Guess single character
        if (correctGuess) {
            // Count occurrences of character in word
            uint32_t count = 0;
            for (char c : word) {
                if (c == ch) count++;
            }
            // Points per char: R1: 10, R2: 15, R3: 20
            uint32_t pointsPerChar = 10;
            if (round == 2) pointsPerChar = 15;
            if (round == 3) pointsPerChar = 20;
            return pointsPerChar * count;
        }
    }
    return 0;
}

void MatchService::startMatch(uint32_t roomId, const std::vector<std::string>& players, const std::string& word) {
    std::lock_guard<std::mutex> lock(matchesMutex);
    
    Match match;
    match.matchId = roomId;
    match.roomId = roomId;
    match.currentRound = 1;
    
    // Get words for all three rounds
    match.round1Word = getRandomWord(1);
    match.round2Word = getRandomWord(2);
    match.round3Word = getRandomWord(3);
    match.currentWord = match.round1Word;
    match.active = true;
    
    // Initialize turn - first player in list starts
    if (!players.empty()) {
        match.currentTurnUsername = players[0];
    }

    for (const auto& p : players) {
        PlayerMatchState state;
        state.username = p;
        state.remainingAttempts = 6;
        state.score = 0;
        match.playerStates[p] = state;
    }

    matches[roomId] = match;
    std::cout << "Match started for room " << roomId 
              << " - Round 1: " << match.round1Word 
              << ", Round 2: " << match.round2Word
              << ", Round 3: " << match.round3Word 
              << " - " << match.currentTurnUsername << " starts first" << std::endl;
}

MatchInfo MatchService::getMatchInfo(uint32_t roomId) {
    std::lock_guard<std::mutex> lock(matchesMutex);
    MatchInfo info;
    info.wordLength = 0;
    info.currentRound = 1;
    
    auto it = matches.find(roomId);
    if (it != matches.end()) {
        info.wordLength = it->second.currentWord.length();
        info.currentRound = it->second.currentRound;
    }
    
    return info;
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
    return getExposedPattern(match.currentWord, opponentState.guessedChars);
}

uint32_t MatchService::getOpponentScore(uint32_t roomId, const std::string& guesserUsername) {
    std::lock_guard<std::mutex> lock(matchesMutex);
    auto it = matches.find(roomId);
    if (it == matches.end()) return 0;
    
    Match& match = it->second;
    
    // Find opponent
    for (const auto& pair : match.playerStates) {
        if (pair.first != guesserUsername) {
            return pair.second.score;
        }
    }
    
    return 0;
}

GuessCharResult MatchService::guessChar(const C2S_GuessChar& request) {
    GuessCharResult result;
    result.success = false;
    result.opponentFd = -1;
    result.scoreGained = 0;

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
    
    // Check if it's this player's turn
    if (match.currentTurnUsername != username) {
        result.errorPacket.message = "Not your turn";
        return result;
    }

    PlayerMatchState& state = match.playerStates[username];
    if (state.finished) {
        result.errorPacket.message = "You already finished";
        return result;
    }

    // Process guess
    bool correct = false;
    if (match.currentWord.find(request.ch) != std::string::npos) {
        correct = true;
        // Add to match's revealed chars (shared between all players)
        match.revealedChars.insert(request.ch);
        
        // Calculate score for correct guess
        result.scoreGained = calculateScore(match.currentRound, true, false, request.ch, match.currentWord);
        state.score += result.scoreGained;
        
        // Track score by round
        if (match.currentRound == 1) state.round1Score += result.scoreGained;
        else if (match.currentRound == 2) state.round2Score += result.scoreGained;
        else if (match.currentRound == 3) state.round3Score += result.scoreGained;
    } else {
        // Wrong guess - no score penalty for character guess, but lose attempt
        if (state.remainingAttempts > 0) state.remainingAttempts--;
    }
    state.guessedChars.insert(request.ch);
    
    // Switch turn to opponent (unless game/round ends)
    std::string opponentUsername;
    for (const auto& pair : match.playerStates) {
        if (pair.first != username) {
            opponentUsername = pair.first;
            break;
        }
    }
    
    // Default: switch turn
    bool switchTurn = true;

    result.success = true;
    result.guesserUsername = username;

    // Find opponent fd
    if (!opponentUsername.empty()) {
        result.opponentFd = AuthService::getInstance().getClientFd(opponentUsername);
    }

    // Check win/loss condition for current round BEFORE creating response packet
    bool won = true;
    for (char c : match.currentWord) {
        if (match.revealedChars.find(c) == match.revealedChars.end()) {
            won = false;
            break;
        }
    }

    if (won) {
        // Player completed current round
        switchTurn = false;  // Don't switch on round completion - player continues
        if (match.currentRound == 1) {
            // Move to round 2 - both players transition together
            match.currentRound = 2;
            match.currentWord = match.round2Word;
            match.revealedChars.clear();  // Clear revealed chars for new round
            
            // Reset ALL players for round 2
            for (auto& playerPair : match.playerStates) {
                playerPair.second.guessedChars.clear();
                playerPair.second.remainingAttempts = 6;
                playerPair.second.finished = false;
            }
            
            std::cout << "Player " << username << " completed Round 1, both players moving to Round 2" << std::endl;
        } else if (match.currentRound == 2) {
            // Move to round 3 - both players transition together
            match.currentRound = 3;
            match.currentWord = match.round3Word;
            match.revealedChars.clear();  // Clear revealed chars for new round
            
            // Reset ALL players for round 3
            for (auto& playerPair : match.playerStates) {
                playerPair.second.guessedChars.clear();
                playerPair.second.remainingAttempts = 6;
                playerPair.second.finished = false;
            }
            
            std::cout << "Player " << username << " completed Round 2, both players moving to Round 3" << std::endl;
        } else {
            // Completed round 3 - game over
            state.finished = true;
            state.won = true;
            std::cout << "Player " << username << " completed Round 3 with score " << state.score << std::endl;
            
            // Note: Game summary will be sent when client requests it via C2S_RequestSummary
            // sendGameSummary(request.room_id);d);
        }
    } else if (state.remainingAttempts == 0) {
        // Out of attempts in current round
        switchTurn = false;  // Don't switch on round completion
        if (match.currentRound == 1) {
            // Move to round 2 - both players transition together
            match.currentRound = 2;
            match.currentWord = match.round2Word;
            match.revealedChars.clear();  // Clear revealed chars for new round
            
            // Reset ALL players for round 2
            for (auto& playerPair : match.playerStates) {
                playerPair.second.guessedChars.clear();
                playerPair.second.remainingAttempts = 6;
                playerPair.second.finished = false;
            }
            
            std::cout << "Player " << username << " ran out of attempts in Round 1, both players moving to Round 2" << std::endl;
        } else if (match.currentRound == 2) {
            // Move to round 3 - both players transition together
            match.currentRound = 3;
            match.currentWord = match.round3Word;
            match.revealedChars.clear();  // Clear revealed chars for new round
            
            // Reset ALL players for round 3
            for (auto& playerPair : match.playerStates) {
                playerPair.second.guessedChars.clear();
                playerPair.second.remainingAttempts = 6;
                playerPair.second.finished = false;
            }
            
            std::cout << "Player " << username << " ran out of attempts in Round 2, both players moving to Round 3" << std::endl;
        } else {
            // Lost in round 3
            state.finished = true;
            state.won = false;
            
            // Note: Game summary will be sent when client requests it
            // sendGameSummary(request.room_id);
        }
    }
    
    // Switch turn if needed
    if (switchTurn && !opponentUsername.empty()) {
        match.currentTurnUsername = opponentUsername;
    }

    // Get fresh state reference after potential reset
    auto& freshState = match.playerStates[username];
    
    // NOW create response packet AFTER round transition with fresh state
    // Use match.revealedChars for pattern (shared between all players)
    result.resultPacket.correct = correct;
    result.resultPacket.remaining_attempts = freshState.remainingAttempts;
    result.resultPacket.exposed_pattern = getExposedPattern(match.currentWord, match.revealedChars);
    result.resultPacket.score_gained = result.scoreGained;
    result.resultPacket.total_score = freshState.score;
    result.resultPacket.current_round = match.currentRound;
    result.resultPacket.is_your_turn = (match.currentTurnUsername == username);
    
    // DEBUG
    std::ofstream debug("/tmp/hangman_backend_debug.txt", std::ios::app);
    if (debug.is_open()) {
        debug << "[MatchService::guessChar] username=" << username 
              << " currentTurnUsername=" << match.currentTurnUsername
              << " is_your_turn=" << result.resultPacket.is_your_turn << "\n";
        debug.close();
    }

    // Create opponent's pattern AFTER round transition
    // SAME pattern for opponent (using shared revealedChars)
    if (!opponentUsername.empty()) {
        auto& opponentState = match.playerStates[opponentUsername];
        result.opponentPattern = getExposedPattern(match.currentWord, match.revealedChars);
        result.opponentScore = opponentState.score;
    }

    return result;
}

GuessWordResult MatchService::guessWord(const C2S_GuessWord& request) {
    GuessWordResult result;
    result.success = false;
    result.gameEnded = false;
    result.opponentFd = -1;
    result.scoreGained = 0;

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
    
    // Check if it's this player's turn
    if (match.currentTurnUsername != username) {
        result.errorPacket.message = "Not your turn";
        return result;
    }
    
    PlayerMatchState& state = match.playerStates[username];
    
    if (state.finished) {
        result.errorPacket.message = "Already finished";
        return result;
    }
    
    // Find opponent username first
    std::string opponentUsername;
    for (const auto& pair : match.playerStates) {
        if (pair.first != username) {
            opponentUsername = pair.first;
            break;
        }
    }
    
    // Default: switch turn
    bool switchTurn = true;

    // Convert guess to uppercase for comparison
    std::string guessUpper = request.word;
    std::transform(guessUpper.begin(), guessUpper.end(), guessUpper.begin(), ::toupper);
    
    bool correct = (guessUpper == match.currentWord);
    
    if (correct) {
        // Correct word guess - bonus points
        uint32_t bonus = calculateScore(match.currentRound, true, true, '\0', match.currentWord);
        result.scoreGained = bonus;
        state.score += bonus;
        
        // Track score by round
        if (match.currentRound == 1) state.round1Score += bonus;
        else if (match.currentRound == 2) state.round2Score += bonus;
        else if (match.currentRound == 3) state.round3Score += bonus;
    } else {
        // Wrong word guess - penalty
        int32_t penalty = 10;
        if (match.currentRound == 2) penalty = 15;
        if (match.currentRound == 3) penalty = 20;
        
        if (state.score >= (uint32_t)penalty) {
            state.score -= penalty;
            // Deduct from current round score
            if (match.currentRound == 1 && state.round1Score >= (uint32_t)penalty) state.round1Score -= penalty;
            else if (match.currentRound == 2 && state.round2Score >= (uint32_t)penalty) state.round2Score -= penalty;
            else if (match.currentRound == 3 && state.round3Score >= (uint32_t)penalty) state.round3Score -= penalty;
        } else {
            state.score = 0;
        }
        result.scoreGained = 0;  // Will show as negative in message
        
        // Also lose an attempt
        if (state.remainingAttempts > 0) state.remainingAttempts--;
    }

    result.success = true;
    result.guesserUsername = username;
    result.resultPacket.correct = correct;
    result.resultPacket.remaining_attempts = state.remainingAttempts;
    result.resultPacket.score_gained = result.scoreGained;
    result.resultPacket.total_score = state.score;
    result.resultPacket.round_complete = false;
    
    // Find opponent
    for (const auto& pair : match.playerStates) {
        if (pair.first != username) {
            result.opponentFd = AuthService::getInstance().getClientFd(pair.first);
            break;
        }
    }
    
    if (correct) {
        // Player guessed the word correctly
        switchTurn = false;  // Player continues after correct guess
        if (match.currentRound == 1) {
            // Move to round 2 - both players transition together
            result.resultPacket.message = "Correct! Moving to Round 2!";
            result.resultPacket.round_complete = true;
            
            match.currentRound = 2;
            match.currentWord = match.round2Word;
            match.revealedChars.clear();  // Clear revealed chars for new round
            
            // Reset ALL players for round 2
            for (auto& playerPair : match.playerStates) {
                playerPair.second.guessedChars.clear();
                playerPair.second.remainingAttempts = 6;
                playerPair.second.finished = false;
            }
            
            // Provide pattern for next round
            result.resultPacket.next_word_pattern = std::string(match.currentWord.length() * 2 - 1, '_');
            for (size_t i = 1; i < match.currentWord.length() * 2 - 1; i += 2) {
                result.resultPacket.next_word_pattern[i] = ' ';
            }
            
            std::cout << "Player " << username << " guessed Round 1 word, both players moving to Round 2" << std::endl;
        } else if (match.currentRound == 2) {
            // Move to round 3 - both players transition together
            result.resultPacket.message = "Correct! Moving to Round 3!";
            result.resultPacket.round_complete = true;
            
            match.currentRound = 3;
            match.currentWord = match.round3Word;
            match.revealedChars.clear();  // Clear revealed chars for new round
            
            // Reset ALL players for round 3
            for (auto& playerPair : match.playerStates) {
                playerPair.second.guessedChars.clear();
                playerPair.second.remainingAttempts = 6;
                playerPair.second.finished = false;
            }
            
            // Provide pattern for next round
            result.resultPacket.next_word_pattern = std::string(match.currentWord.length() * 2 - 1, '_');
            for (size_t i = 1; i < match.currentWord.length() * 2 - 1; i += 2) {
                result.resultPacket.next_word_pattern[i] = ' ';
            }
            
            std::cout << "Player " << username << " guessed Round 2 word, both players moving to Round 3" << std::endl;
        } else {
            // Completed round 3
            result.resultPacket.message = "Correct! You completed all 3 rounds with score " + std::to_string(state.score) + "!";
            state.finished = true;
            state.won = true;
            result.gameEnded = true;
            
            std::cout << "Player " << username << " completed all rounds with score " << state.score << std::endl;
            
            // Note: Game summary will be sent when client requests it
            // sendGameSummary(request.room_id);
        }
    } else {
        // Wrong word guess - calculate penalty based on round
        int32_t penalty = 10;
        if (match.currentRound == 2) penalty = 15;
        if (match.currentRound == 3) penalty = 20;
        
        result.resultPacket.message = "Incorrect! Lost " + std::to_string(penalty) + " points";
        
        if (state.remainingAttempts == 0) {
            switchTurn = false;  // Don't switch on round transition
            if (match.currentRound == 1) {
                // Out of attempts in round 1, move to round 2 - both players transition
                result.resultPacket.message += ". Out of attempts! Moving to Round 2.";
                result.resultPacket.round_complete = true;
                
                match.currentRound = 2;
                match.currentWord = match.round2Word;
                match.revealedChars.clear();  // Clear revealed chars for new round
                
                // Reset ALL players for round 2
                for (auto& playerPair : match.playerStates) {
                    playerPair.second.guessedChars.clear();
                    playerPair.second.remainingAttempts = 6;
                    playerPair.second.finished = false;
                }
                
                result.resultPacket.next_word_pattern = std::string(match.currentWord.length() * 2 - 1, '_');
                for (size_t i = 1; i < match.currentWord.length() * 2 - 1; i += 2) {
                    result.resultPacket.next_word_pattern[i] = ' ';
                }
            } else if (match.currentRound == 2) {
                // Out of attempts in round 2, move to round 3 - both players transition
                result.resultPacket.message += ". Out of attempts! Moving to Round 3.";
                result.resultPacket.round_complete = true;
                
                match.currentRound = 3;
                match.currentWord = match.round3Word;
                match.revealedChars.clear();  // Clear revealed chars for new round
                
                // Reset ALL players for round 3
                for (auto& playerPair : match.playerStates) {
                    playerPair.second.guessedChars.clear();
                    playerPair.second.remainingAttempts = 6;
                    playerPair.second.finished = false;
                }
                
                result.resultPacket.next_word_pattern = std::string(match.currentWord.length() * 2 - 1, '_');
                for (size_t i = 1; i < match.currentWord.length() * 2 - 1; i += 2) {
                    result.resultPacket.next_word_pattern[i] = ' ';
                }
            } else {
                // Out of attempts in round 3 - game over
                state.finished = true;
                state.won = false;
                result.gameEnded = true;
                result.resultPacket.message = "Out of attempts! Final score: " + std::to_string(state.score);
                
                // Note: Game summary will be sent when client requests it
                // sendGameSummary(request.room_id);
            }
        }
    }
    
    // Update current_round in response packet AFTER all round transitions
    result.resultPacket.current_round = match.currentRound;
    
    // Switch turn if needed
    if (switchTurn && !opponentUsername.empty()) {
        match.currentTurnUsername = opponentUsername;
    }
    
    // Set is_your_turn field
    result.resultPacket.is_your_turn = (match.currentTurnUsername == username);

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
    
    // Get scores from match if available
    auto userStateIt = match.playerStates.find(username);
    auto oppStateIt = match.playerStates.find(opponentName);
    
    if (userStateIt != match.playerStates.end()) {
        // Save with detailed scores
        saveHistoryWithScores(username, opponentName, request.result_code, 
                             userStateIt->second.round1Score, 
                             userStateIt->second.round2Score, 
                             userStateIt->second.round3Score);
    } else {
        // Fallback to old format if no match data
        saveHistory(username, opponentName, request.result_code, summary);
    }

    // If resignation, opponent wins
    if (request.result_code == 0) {
        AuthService::getInstance().updateUserStats(opponentName, true, 10);
        
        if (oppStateIt != match.playerStates.end()) {
            // Save with detailed scores
            saveHistoryWithScores(opponentName, username, 1, 
                                 oppStateIt->second.round1Score, 
                                 oppStateIt->second.round2Score, 
                                 oppStateIt->second.round3Score);
        } else {
            saveHistory(opponentName, username, 1, "Opponent resigned");
        }
    } 
    // If draw, update opponent too (assuming both agreed)
    else if (request.result_code == 3) {
        AuthService::getInstance().updateUserStats(opponentName, false, 1);
        
        if (oppStateIt != match.playerStates.end()) {
            saveHistoryWithScores(opponentName, username, 3, 
                                 oppStateIt->second.round1Score, 
                                 oppStateIt->second.round2Score, 
                                 oppStateIt->second.round3Score);
        } else {
            saveHistory(opponentName, username, 3, "Draw");
        }
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

    // Note: We no longer automatically send game summary here
    // Client will request it manually when user presses "View Summary"
    
    // Clean up match if both finished?
    // For now keep it simple.
    
    return result;
}

S2C_GameSummary MatchService::requestSummary(const C2S_RequestSummary& request) {
    S2C_GameSummary summary;
    
    std::string username;
    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        // Return empty summary on auth failure
        return summary;
    }

    std::lock_guard<std::mutex> lock(matchesMutex);
    auto it = matches.find(request.room_id);
    if (it == matches.end()) {
        // Match not found - return empty summary
        return summary;
    }
    
    Match& match = it->second;
    if (match.playerStates.size() != 2) {
        // Invalid match state
        return summary;
    }
    
    // Get both players
    auto playerIt = match.playerStates.begin();
    std::string player1 = playerIt->first;
    PlayerMatchState& state1 = playerIt->second;
    playerIt++;
    std::string player2 = playerIt->first;
    PlayerMatchState& state2 = playerIt->second;
    
    // Create summary packet
    summary.player1_username = player1;
    summary.player1_round1_score = state1.round1Score;
    summary.player1_round2_score = state1.round2Score;
    summary.player1_round3_score = state1.round3Score;
    summary.player1_total_score = state1.score;
    
    summary.player2_username = player2;
    summary.player2_round1_score = state2.round1Score;
    summary.player2_round2_score = state2.round2Score;
    summary.player2_round3_score = state2.round3Score;
    summary.player2_total_score = state2.score;
    
    // Determine winner
    if (state1.score > state2.score) {
        summary.winner_username = player1;
    } else if (state2.score > state1.score) {
        summary.winner_username = player2;
    } else {
        summary.winner_username = "";  // Draw
    }
    
    return summary;
}

void MatchService::saveHistory(const std::string& username, const std::string& opponent, uint8_t result, const std::string& summary) {
    std::string dir = "database/history";
    try {
        std::filesystem::create_directories(dir);
    } catch (...) {}

    std::time_t t = std::time(nullptr);
    std::string filepath = dir + "/" + username + ".txt";
    
    std::ofstream file(filepath, std::ios::app);  // Append mode
    if (file.is_open()) {
        // Format: datetime:opponent:result:summary
        char datetime[20];
        std::strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        file << datetime << ":" << opponent << ":" << (int)result << ":" << summary << "\n";
        file.close();
    }
}

void MatchService::saveHistoryWithScores(const std::string& username, const std::string& opponent, 
                                         uint8_t result, uint32_t r1Score, uint32_t r2Score, uint32_t r3Score) {
    std::string dir = "database/history";
    try {
        std::filesystem::create_directories(dir);
    } catch (...) {}

    std::time_t t = std::time(nullptr);
    std::string filepath = dir + "/" + username + ".txt";
    
    std::ofstream file(filepath, std::ios::app);  // Append mode
    if (file.is_open()) {
        // Format: datetime:opponent:win/lose:r1_score:r2_score:r3_score
        char datetime[20];
        std::strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        std::string resultStr = (result == 1) ? "win" : (result == 0) ? "lose" : "draw";
        file << datetime << ":" << opponent << ":" << resultStr << ":" 
             << r1Score << ":" << r2Score << ":" << r3Score << "\n";
        file.close();
    }
}

void MatchService::sendGameSummary(uint32_t roomId) {
    std::lock_guard<std::mutex> lock(matchesMutex);
    auto it = matches.find(roomId);
    if (it == matches.end()) return;
    
    Match& match = it->second;
    if (match.playerStates.size() != 2) return;
    
    // Get both players
    auto playerIt = match.playerStates.begin();
    std::string player1 = playerIt->first;
    PlayerMatchState& state1 = playerIt->second;
    playerIt++;
    std::string player2 = playerIt->first;
    PlayerMatchState& state2 = playerIt->second;
    
    // Create summary packet
    S2C_GameSummary summary;
    summary.player1_username = player1;
    summary.player1_round1_score = state1.round1Score;
    summary.player1_round2_score = state1.round2Score;
    summary.player1_round3_score = state1.round3Score;
    summary.player1_total_score = state1.score;
    
    summary.player2_username = player2;
    summary.player2_round1_score = state2.round1Score;
    summary.player2_round2_score = state2.round2Score;
    summary.player2_round3_score = state2.round3Score;
    summary.player2_total_score = state2.score;
    
    // Determine winner
    if (state1.score > state2.score) {
        summary.winner_username = player1;
    } else if (state2.score > state1.score) {
        summary.winner_username = player2;
    } else {
        summary.winner_username = "";  // Draw
    }
    
    // Send to both players
    int fd1 = AuthService::getInstance().getClientFd(player1);
    int fd2 = AuthService::getInstance().getClientFd(player2);
    
    auto summaryBytes = summary.to_bytes();
    
    if (fd1 != -1) {
        send(fd1, summaryBytes.data(), summaryBytes.size(), 0);
    }
    if (fd2 != -1) {
        send(fd2, summaryBytes.data(), summaryBytes.size(), 0);
    }
    
    // Save history for both players with detailed scores
    uint8_t result1 = (state1.score > state2.score) ? 1 : (state1.score < state2.score ? 0 : 2);  // 1=win, 0=loss, 2=draw
    uint8_t result2 = (state2.score > state1.score) ? 1 : (state2.score < state1.score ? 0 : 2);
    
    saveHistoryWithScores(player1, player2, result1, state1.round1Score, state1.round2Score, state1.round3Score);
    saveHistoryWithScores(player2, player1, result2, state2.round1Score, state2.round2Score, state2.round3Score);
    
    std::cout << "Game summary sent for room " << roomId << " - Winner: " << 
                 (summary.winner_username.empty() ? "Draw" : summary.winner_username) << std::endl;
}

} // namespace hangman
