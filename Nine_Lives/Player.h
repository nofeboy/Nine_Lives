#pragma once
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp> // JSON 라이브러리 포함

struct Player {
    int strength = 3;
    int hacking = 3;
    int hp = 7;
    int sanity = 7;
    int money = 7;
    int cloneBodies = 0;

    std::map<std::string, int> items;
    std::map<std::string, int> information;

    void addItem(const std::string& itemName, int count = 1);
    void addInformation(const std::string& infoName, int count = 1);
    std::string getRandomItem() const;
    void removeItem(const std::string& itemName);

    bool isDead() const { return hp <= 0 || sanity <= 0; }
    bool isCompletelyDead() const { return cloneBodies <= 0; }

    void useCloneBody();
    void resetToDefault();
    void applyStatLimits();
};

// Player 구조체를 위한 JSON 직렬화/역직렬화 함수
void to_json(nlohmann::json& j, const Player& p);
void from_json(const nlohmann::json& j, Player& p);