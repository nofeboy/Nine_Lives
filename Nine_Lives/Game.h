#pragma once
#include <string>
#include "Player.h"
#include "EventManager.h"
#include "Input.h"
#include <deque>
#include <windows.h>
#include <unordered_set>

enum class GameState {
    MAIN_MENU,
    PLAYING,
    REVIVING,
    GAME_OVER
};

class Game {
public:
    void run();
    static bool isChoiceLocked(const Choice& choice, const Player& player);

private:
    Player player;
    EventManager eventManager;
    std::string currentEventId = "intro_1";
    GameState currentState = GameState::MAIN_MENU; // 현재 게임 상태 변수 추가
    int mainMenuSelection = 0; // 메인 메뉴 선택 인덱스

    Direction lastDir = NONE;
    bool previewMode = false;
    bool animateCard = true;

    void processChoice(int choiceIndex);
    void saveGame();
    bool loadGame();

    std::string forceNextEventId;
    std::deque<std::string> recentEvents;
    std::string getNextEventId();
    bool checkEventCondition(const Event& ev);
    bool showReviveCutscene = false;

    bool turnStarted = false;
    int turnCount = 0;
    int scenarioCount = 0;
    std::unordered_set<std::string> playerHistory;

    int getPlayerStat(const Player& player, const std::string& statName);
    bool isConditionMet(const Condition& cond, const Player& player, int turnCount);

    std::unordered_map<std::string, std::vector<int>> choiceTurnHistory;

    bool autoAdvancing = false;
    ULONGLONG autoUntilMs = 0;
    std::string autoNextEventId;
    bool oneShotRendered = false;
};