#include "Player.h"
#include <random>
#include <iostream>

void Player::addItem(const std::string& itemName, int count) {
    items[itemName] += count;
    if (items[itemName] <= 0) {
        items.erase(itemName);
    }
}

void Player::addInformation(const std::string& infoName, int count) {
    information[infoName] += count;
    if (information[infoName] <= 0) {
        information.erase(infoName);
    }
}

// ✅ 랜덤 아이템 선택 함수 (현재 사용되지 않지만 유지)
std::string Player::getRandomItem() const {
    if (items.empty()) return "";

    std::vector<std::string> itemNames;
    for (const auto& item : items) {
        if (item.second > 0) {
            itemNames.push_back(item.first);
        }
    }

    if (itemNames.empty()) return "";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, itemNames.size() - 1);

    return itemNames[dis(gen)];
}

void Player::removeItem(const std::string& itemName) {
    auto it = items.find(itemName);
    if (it != items.end()) {
        it->second--;
        if (it->second <= 0) {
            items.erase(it);
        }
    }
}

// ✅ 클론 바디 사용 로직 (아이템 드롭 기능 제거)
void Player::useCloneBody() {
    if (cloneBodies > 0) {
        //cloneBodies--;

        // ✅ 스탯 리셋
        hp = 5;
        sanity = 5;
        money = 0;
        strength = 1;
        hacking = 1;
        items.clear();
        information.clear();
    }
}

// ✅ 완전 리셋 (게임 재시작용)
void Player::resetToDefault() {
    strength = 0;
    hacking = 0;
    hp = 7;
    sanity = 7;
    money = 3;
    cloneBodies = 9;
    items.clear();
    information.clear();
}

// ✅ 스탯 제한 적용 함수
void Player::applyStatLimits() {
    hp = std::max(0, std::min(7, hp));
    sanity = std::max(0, std::min(7, sanity));
    strength = std::max(0, std::min(7, strength));
    hacking = std::max(0, std::min(7, hacking));
    money = std::max(0, money);
    cloneBodies = std::max(0, std::min(9, cloneBodies));
}