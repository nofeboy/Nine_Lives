#pragma once
#include <string>
#include "Player.h"
#include "EventManager.h"
#include <deque>
#include <unordered_set>

class Game {
public:
    void run();
private:
    Player player;
    EventManager eventManager;
    std::string currentEventId = "intro_1";
    void processChoice(int choiceIndex);
    void setConsoleSize(int width, int height);
    void disableQuickEdit();
    std::string forceNextEventId;
    std::deque<std::string> recentEvents;
    std::string getNextEventId();
    bool checkEventCondition(const Event& ev);

    int turnCount = 0;  // ✅ 추가
    std::unordered_set<std::string> playerHistory; // ✅ 선택 기록 저장
};
