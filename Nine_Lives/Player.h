#pragma once
#include <map>
#include <string>
#include <vector>

struct Player {
    int strength = 0;
    int hacking = 0;
    int hp = 7;
    int sanity = 7;
    int money = 3;
    int cloneBodies = 9;  // 클론 바디 개수

    // 아이템 (물건 - 사망시 소실)
    std::map<std::string, int> items;

    // 정보 (게임오버시 유지, 사망시만 소실)
    std::map<std::string, int> information;

    // 아이템 관리
    void addItem(const std::string& itemName, int count = 1);
    void addInformation(const std::string& infoName, int count = 1);
    std::string getRandomItem() const;
    void removeItem(const std::string& itemName);

    // 상태 체크
    bool isDead() const { return hp <= 0 || sanity <= 0; }
    bool isCompletelyDead() const { return cloneBodies <= 0; }

    // 클론 바디 소모 및 리셋
    void useCloneBody();
};