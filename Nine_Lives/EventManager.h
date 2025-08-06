#pragma once
#include <unordered_map>
#include <string>
#include <vector>

struct Choice {
    std::string text;
    int reqStrength;
    int reqHacking;
    int hpDelta;
    int sanityDelta;
    int moneyDelta;
    int strengthDelta = 0;
    int hackingDelta = 0;
    std::string nextEventId;

    // 아이템 효과 추가
    std::vector<std::pair<std::string, int>> itemGains;
    std::vector<std::pair<std::string, int>> informationGains;
    std::vector<std::string> requiredItems;  // 필요 아이템
};

struct StatCondition {
    int min = 0;
};
struct Condition {
    int minTurns = 0;
    std::unordered_map<std::string, StatCondition> stats;
    std::vector<std::string> requiredChoices;
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