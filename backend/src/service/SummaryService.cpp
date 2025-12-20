#include "service/SummaryService.h"
#include "service/AuthService.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

using namespace std::filesystem;

namespace hangman {

static SummaryService* g_summaryService = nullptr;

SummaryService& SummaryService::getInstance() {
    if (!g_summaryService) {
        g_summaryService = new SummaryService();
    }
    return *g_summaryService;
}

S2C_HistoryList SummaryService::getHistory(const C2S_RequestHistory& request) {
    S2C_HistoryList response;
    std::string username;

    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        return response;
    }

    std::string dirPath = "database/history/" + username;
    if (!exists(dirPath)) {
        return response;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            std::ifstream file(entry.path());
            std::string line;
            if (std::getline(file, line)) {
                // Format: match_id:opponent:result:timestamp:summary
                std::istringstream iss(line);
                std::string segment;
                std::vector<std::string> parts;
                while (std::getline(iss, segment, ':')) {
                    parts.push_back(segment);
                }

                if (parts.size() >= 5) {
                    S2C_HistoryList::Entry e;
                    try {
                        e.match_id = std::stoul(parts[0]);
                        e.opponent = parts[1];
                        e.result_code = std::stoi(parts[2]);
                        e.timestamp = std::stoul(parts[3]);
                        e.summary = parts[4];
                        response.entries.push_back(e);
                    } catch (...) {}
                }
            }
        }
    }
    
    // Sort by timestamp desc
    std::sort(response.entries.begin(), response.entries.end(), 
        [](const S2C_HistoryList::Entry& a, const S2C_HistoryList::Entry& b) {
            return a.timestamp > b.timestamp;
        });

    return response;
}

S2C_Leaderboard SummaryService::getLeaderboard(const C2S_RequestLeaderboard& request) {
    S2C_Leaderboard response;
    std::string username;

    if (!AuthService::getInstance().validateSession(request.session_token, username)) {
        return response;
    }

    auto users = AuthService::getInstance().getAllUsers();
    
    // Sort by points desc
    std::sort(users.begin(), users.end(), [](const User& a, const User& b) {
        return a.total_points > b.total_points;
    });

    // Take top 10
    int count = 0;
    for (const auto& u : users) {
        if (count >= 10) break;
        S2C_Leaderboard::Row row;
        row.username = u.username;
        row.wins = u.wins;
        row.losses = 0; // Not tracked in User struct currently
        row.draws = 0;  // Not tracked
        response.rows.push_back(row);
        count++;
    }

    return response;
}

} // namespace hangman
