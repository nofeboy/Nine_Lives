#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <climits>

struct StatCondition {
    int min = INT_MIN;
    int max = INT_MAX;
};


struct Condition {
    int minTurns = 0;
    std::unordered_map<std::string, StatCondition> stats;
    std::vector<std::string> requiredChoices;

    std::string stat;
    std::string item;
    std::string info;
    int min = 0;
    bool hasTurn = false;
    int turnValue = 0;

    std::string afterChoice;  // ★ 추가: 특정 선택 이후
    int turnsPassed = 0;      // ★ 추가: 그로부터 n턴 후
    std::vector<std::string> excludedIfChoiceMade; // 이 선택을 한 적 있으면 등장 X
    int minScenarios = 0; // ★ 추가
    std::vector<std::string> excludedIfInfoOwned; // 특정 정보가 있으면 등장 X

};


struct Outcome {
    std::vector<Condition> conditions;  // 만족 조건들

    // 🔹 분기용
    std::string nextEventId;

    // 🔹 능력치 변화
    int moneyDelta = 0;
    int hpDelta = 0;
    int sanityDelta = 0;
    int strengthDelta = 0;
    int hackingDelta = 0;
    int cloneDelta = 0;

    // 🔹 아이템/정보 변화
    std::unordered_map<std::string, int> itemChanges;
    std::unordered_map<std::string, int> infoChanges;
};


struct Choice {
    std::string text;
    int reqStrength;
    int reqHacking;
    int hpDelta;
    int sanityDelta;
    int moneyDelta;
    int strengthDelta = 0;
    int hackingDelta = 0;
    int cloneDelta = 0;
    std::string nextEventId;

    // 아이템 효과 추가
    std::vector<std::pair<std::string, int>> itemGains;
    std::vector<std::pair<std::string, int>> informationGains;
    std::vector<std::string> requiredItems;  // 필요 아이템
    std::vector<Outcome> outcomes; // ★추가!

};




struct Event {
    std::string id;
    std::string description;
    std::string type;
    Condition condition; // ✅ 추가
    std::vector<Choice> choices;
};

class EventManager {
public:
    void loadEvents(const std::string& filename);
    Event& getEvent(const std::string& id);
    std::vector<std::string> getForcedEvents(const std::string& trigger);

    std::vector<std::string> randomPool;   
    std::unordered_map<std::string, Event> events;


private:
    std::unordered_map<std::string, std::vector<std::string>> forcedEvents;
};