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
    std::string nextEventId;

    // 아이템 효과 추가
    std::vector<std::pair<std::string, int>> itemGains;
    std::vector<std::pair<std::string, int>> informationGains;
    std::vector<std::string> requiredItems;  // 필요 아이템
};

struct Event {
    std::string id;
    std::string description;
    std::vector<Choice> choices;
};

class EventManager {
public:
    void loadEvents(const std::string& filename);
    Event& getEvent(const std::string& id);
private:
    std::unordered_map<std::string, Event> events;
};