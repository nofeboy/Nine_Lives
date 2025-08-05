#include "Player.h"
#include <random>
#include <iostream>

void Player::addItem(const std::string& itemName, int count) {
    items[itemName] += count;
}

void Player::addInformation(const std::string& infoName, int count) {
    information[infoName] += count;
}

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
    if (items.find(itemName) != items.end()) {
        items[itemName]--;
        if (items[itemName] <= 0) {
            items.erase(itemName);
        }
    }
}

void Player::useCloneBody() {
    if (cloneBodies > 0) {
        cloneBodies--;

        // 아이템 드랍
        std::string droppedItem = getRandomItem();
        if (!droppedItem.empty()) {
            removeItem(droppedItem);
        }

        // 스탯 리셋
        hp = 7;
        sanity = 7;
        money = 3;
    }
}
