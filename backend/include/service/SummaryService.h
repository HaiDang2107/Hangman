#pragma once

#include "protocol/packets.h"
#include <vector>

namespace hangman {

class SummaryService {
public:
    static SummaryService& getInstance();

    SummaryService(const SummaryService&) = delete;
    SummaryService& operator=(const SummaryService&) = delete;

    S2C_HistoryList getHistory(const C2S_RequestHistory& request);
    S2C_Leaderboard getLeaderboard(const C2S_RequestLeaderboard& request);

private:
    SummaryService() = default;
    ~SummaryService() = default;
};

} // namespace hangman
