#include "Game.h"
#include "Renderer.h"
#include "Input.h"
#include "ConsoleUtils.h"
#include <windows.h>
#include <chrono>
#include <thread>
#include <iostream>

using namespace std;

void Game::setConsoleSize(int width, int height) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT DisplayArea = { 0, 0, (short)(width - 1), (short)(height - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);
    COORD bufferSize = { (short)width, (short)height };
    SetConsoleScreenBufferSize(hOut, bufferSize);
}

void Game::disableQuickEdit() {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prev_mode;
    GetConsoleMode(hInput, &prev_mode);
    SetConsoleMode(hInput, prev_mode & ~ENABLE_QUICK_EDIT_MODE);
}

void Game::run() {
    ConsoleUtils::enableANSI();
    ConsoleUtils::setUTF8();
    ConsoleUtils::setConsoleSize(120, 40);
    ConsoleUtils::disableResize();
    ConsoleUtils::hideCursor();

    eventManager.loadEvents("events.json");

    Direction lastDir = NONE;
    bool previewMode = false;
    bool animateCard = true;

    while (true) {
        auto& ev = eventManager.getEvent(currentEventId);

        if (ev.choices.empty()) {
            Renderer::renderEventFull(ev, player, animateCard); // 최종 이벤트
            if (player.isCompletelyDead()) Renderer::showCompleteGameOver();
            else Renderer::showGameOver();
            break;
        }

        if (!previewMode) {
            Renderer::renderEventFull(ev, player, animateCard); // 카드 + HUD 전체 출력
            animateCard = false;
        }
        else {
            Renderer::updateChoicesOnly(ev, player, lastDir); // 선택지만 갱신
        }

        Direction dir = Input::getDirection();
        if (dir == NONE) continue;

        int idx = -1;
        if (dir == RIGHT && ev.choices.size() > 0) idx = 0;
        else if (dir == LEFT && ev.choices.size() > 1) idx = 1;
        else if (dir == UP && ev.choices.size() > 2) idx = 2;
        else if (dir == DOWN && ev.choices.size() > 3) idx = 3;

        if (!previewMode) {
            if (idx == -1) continue;
            lastDir = dir;
            previewMode = true;
        }
        else if (previewMode && dir == lastDir) {
            if (idx == -1) continue;

            // LOCKED 체크
            bool locked = (player.strength < ev.choices[idx].reqStrength || player.hacking < ev.choices[idx].reqHacking);
            for (const string& reqItem : ev.choices[idx].requiredItems) {
                if (player.items.find(reqItem) == player.items.end() || player.items.at(reqItem) <= 0) {
                    locked = true;
                    break;
                }
            }

            if (locked) {
                //Renderer::showLockedWarning();
                previewMode = false;
                continue;
            }

            Renderer::eraseCardText(ev.description); // 기존 카드 내용만 지움
            processChoice(idx);
            previewMode = false;
            animateCard = true;
        }
        else {
            lastDir = dir;
        }
    }
}

void Game::processChoice(int choiceIndex) {
    auto& choice = eventManager.getEvent(currentEventId).choices[choiceIndex];

    // 필요 아이템 소모
    for (const string& reqItem : choice.requiredItems) {
        if (player.items.find(reqItem) != player.items.end() && player.items[reqItem] > 0) {
            player.removeItem(reqItem);
        }
    }

    // 스탯 변화
    player.hp += choice.hpDelta;
    player.sanity += choice.sanityDelta;
    player.money += choice.moneyDelta;

    // 스탯 클램핑 (하한/상한)
    if (player.hp > 7) player.hp = 7;
    if (player.hp < 0) player.hp = 0;
    if (player.sanity > 7) player.sanity = 7;
    if (player.sanity < 0) player.sanity = 0;
    if (player.strength > 7) player.strength = 7;
    if (player.hacking > 7) player.hacking = 7;
    if (player.money < 0) player.money = 0;

    // 아이템 획득
    for (const auto& item : choice.itemGains) {
        player.addItem(item.first, item.second);
    }

    // 정보 획득
    for (const auto& info : choice.informationGains) {
        player.addInformation(info.first, info.second);
    }

    if (player.isDead()) {
        if (player.cloneBodies > 0) {
            Renderer::showCloneRevival();
            std::string droppedItem = player.getRandomItem();
            if (!droppedItem.empty()) {
                Renderer::showItemDrop(droppedItem);
            }
            player.useCloneBody();
            currentEventId = "clone_revival";
        }
        else {
            currentEventId = "complete_death";
        }
        return;
    }


    // 다음 이벤트로 이동
    currentEventId = choice.nextEventId;
}
