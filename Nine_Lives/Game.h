#pragma once
#include <string>
#include "Player.h"
#include "EventManager.h"
#include <deque>
#include <unordered_set>


class Game {
public:
    void run();
    static bool isChoiceLocked(const Choice& choice, const Player& player);

private:
    Player player;
    EventManager eventManager;
    std::string currentEventId = "intro_1";
    void processChoice(int choiceIndex);
    void setConsoleSize(int width, int height);
    std::string forceNextEventId;
    std::deque<std::string> recentEvents;
    std::string getNextEventId();
    bool checkEventCondition(const Event& ev);
    bool showReviveCutscene = false;

    bool turnStarted = false;
    int turnCount = 0;  // ✅ 추가
    int scenarioCount = 0;
    std::unordered_set<std::string> playerHistory; // ✅ 선택 기록 저장

    int getPlayerStat(const Player& player, const std::string& statName);
    bool isConditionMet(const Condition& cond, const Player& player, int turnCount);

    std::unordered_map<std::string, std::vector<int>> choiceTurnHistory; // ★ 선택지 별 선택 턴 기록

};
