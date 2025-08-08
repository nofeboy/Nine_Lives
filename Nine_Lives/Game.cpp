#include "Game.h"
#include "Renderer.h"
#include "Input.h"
#include "ConsoleUtils.h"
#include <windows.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <conio.h>

using namespace std;

// 선택지 잠금 체크 함수
bool Game::isChoiceLocked(const Choice& choice, const Player& player) {
    // 스탯 요구사항 체크
    if (player.strength < choice.reqStrength || player.hacking < choice.reqHacking) {
        return true;
    }

    // 필수 아이템 체크
    for (const string& reqItem : choice.requiredItems) {
        auto it = player.items.find(reqItem);
        if (it == player.items.end() || it->second <= 0) {
            return true;
        }
    }

    return false;
}

void Game::run() {
    srand((unsigned int)time(NULL));

    ConsoleUtils::enableANSI();
    ConsoleUtils::setUTF8();
    ConsoleUtils::setConsoleSize(120, 40);
    ConsoleUtils::disableResize();
    ConsoleUtils::hideCursor();

    eventManager.loadEvents("events.json");

    // ★ run() 내부 상태만 사용(멤버 추가 없음)
    std::string lastRenderedEventId; // 이벤트 바뀜 감지 → 첫 렌더 애니 보장
    bool flushOnce = false;          // 전이 직후 1회만 키버퍼 비우기

    while (true) {
        switch (currentState) {
        case GameState::MAIN_MENU: {
            Renderer::renderMainMenu();
            int key = _getch();
            if (key == 27) return; // ESC 종료

            // 게임 시작 초기화
            //player.resetToDefault();
            turnStarted = false;
            turnCount = 0;
            scenarioCount = 0;
            playerHistory.clear();
            choiceTurnHistory.clear();
            currentEventId = "intro_1";

            // 상태
            animateCard = true;      // ★ 첫 이벤트 애니 보장
            previewMode = false;
            lastDir = NONE;
            lastRenderedEventId.clear();

            flushOnce = true;        // ★ 전이 직후 1회 버퍼 정리
            currentState = GameState::PLAYING;
            break;
        }

        case GameState::PLAYING: {
            // 전이 직후 한 번만 버퍼 비움(스킵 방지)
            if (flushOnce) {
                while (_kbhit()) _getch();
                flushOnce = false;
            }

            // ★ revive_story 최우선 처리(다른 입력 전에)
            if (currentEventId == "revive_story") {
                auto& ev = eventManager.getEvent(currentEventId);

                // 이벤트가 바뀌었으면 무조건 애니
                bool firstTime = (lastRenderedEventId != currentEventId);
                if (!previewMode) {
                    Renderer::renderEventFull(ev, player, /*animate*/ firstTime || animateCard,
                        turnCount, scenarioCount);
                    lastRenderedEventId = currentEventId;
                    animateCard = false;
                }

                std::cout << "\n\n(PRESS ANY KEY TO CONTINUE)\n";
                _getch(); // 여기서만 입력 소비

                // 다음 이벤트로
                currentEventId = getNextEventId();   // 필요시 고정 ID
                animateCard = true;               // 다음 이벤트 첫 렌더 애니 가능
                previewMode = false;
                lastDir = NONE;
                lastRenderedEventId.clear();
                flushOnce = true;                    // 전이 직후 1회 버퍼 정리
                continue;
            }

            // 사망 진입(반복 전환 방지)
            if (player.isDead()) {
                if (currentEventId != "death_notice") {
                    currentEventId = "death_notice";
                    animateCard = true;
                    lastRenderedEventId.clear();
                }
            }

            auto& ev = eventManager.getEvent(currentEventId);

            // 선택지 없는 이벤트(엔딩/사망 알림 등)
            if (ev.choices.empty()) {
                bool firstTime = (lastRenderedEventId != currentEventId);
                Renderer::renderEventFull(ev, player, /*animate*/ firstTime || animateCard,
                    turnCount, scenarioCount);
                lastRenderedEventId = currentEventId;
                animateCard = false;

                // death_notice 자동 분기 or 일반 엔딩
                if (currentEventId == "death_notice") {
                    std::cout << "\n\n(PRESS ANY KEY)\n";
                    _getch();
                    if (player.isCompletelyDead()) {
                        currentState = GameState::GAME_OVER;
                    }
                    else {
                        currentState = GameState::REVIVING;
                    }
                }
                else {
                    std::cout << "\n\n(PRESS ANY KEY TO RETURN TO MAIN MENU)\n";
                    _getch();
                    currentState = GameState::MAIN_MENU;
                }
                continue;
            }

            // 일반 이벤트 렌더(첫 렌더는 애니)
            if (!previewMode) {
                bool firstTime = (lastRenderedEventId != currentEventId);
                Renderer::renderEventFull(ev, player, /*animate*/ firstTime || animateCard,
                    turnCount, scenarioCount);
                lastRenderedEventId = currentEventId;
                animateCard = false;
            }
            else {
                Renderer::updateChoicesOnly(ev, player, lastDir);
            }

            // 방향키 입력
            Direction dir = Input::getDirection();
            if (dir == NONE) break;

            int idx = -1;
            if (dir == RIGHT && ev.choices.size() > 0) idx = 0;
            else if (dir == LEFT && ev.choices.size() > 1) idx = 1;
            else if (dir == UP && ev.choices.size() > 2) idx = 2;
            else if (dir == DOWN && ev.choices.size() > 3) idx = 3;

            if (!previewMode) {
                if (idx != -1) { lastDir = dir; previewMode = true; }
            }
            else if (dir == lastDir) {
                if (idx != -1) {
                    if (isChoiceLocked(ev.choices[idx], player)) {
                        Renderer::showLockedWarning();
                        previewMode = false;
                    }
                    else {
                        Renderer::eraseCardText(ev.description);
                        processChoice(idx);
                        previewMode = false;
                        animateCard = true;     // 다음 이벤트 첫 렌더 애니
                        lastRenderedEventId.clear();
                    }
                }
            }
            else {
                if (idx != -1) lastDir = dir;
                else previewMode = false;
            }
            break;
        }

        case GameState::REVIVING:
            Renderer::showReviveAnimation();
            while (_kbhit()) _getch(); // ★ 컷신 직후 잔여키 제거(1회)
            player.useCloneBody();
            currentEventId = "revive_story";
            animateCard = true;        // revive_story 첫 렌더 애니
            previewMode = false;
            lastDir = NONE;
            // 다음 루프로 넘어가면 PLAYING 최상단 revive_story 블록이 처리
            currentState = GameState::PLAYING;
            continue;

        case GameState::GAME_OVER:
            Renderer::showGameOverAnimation();
            return;
        }
    }
}

void Game::processChoice(int choiceIndex) {
    auto& ev = eventManager.getEvent(currentEventId);
    auto& choice = ev.choices[choiceIndex];

    // 부활/게임오버 트리거 확인
    if (choice.nextEventId == "__TRIGGER_REVIVAL__") {
        // ★ 수치 변화 먼저 적용
        player.money += choice.moneyDelta;
        player.hp += choice.hpDelta;
        player.sanity += choice.sanityDelta;
        player.strength += choice.strengthDelta;
        player.hacking += choice.hackingDelta;
        player.cloneBodies += choice.cloneDelta;

        for (const auto& item : choice.itemGains) {
            player.addItem(item.first, item.second);
        }
        for (const auto& info : choice.informationGains) {
            player.addInformation(info.first, info.second);
        }

        // ★ 스탯 제한 반영
        player.applyStatLimits();

        // 상태 변경
        if (player.isCompletelyDead()) {
            currentState = GameState::GAME_OVER;
        }
        else {
            currentState = GameState::REVIVING;
        }
        return;
    }

    // --- 이하 로직은 기존과 동일 ---
    bool goingRandom = false;

    std::string choiceKey = ev.id + ":" + choice.text;
    playerHistory.insert(choiceKey);
    playerHistory.insert(ev.id);

    if (ev.type == "random_once") {
        auto& pool = eventManager.randomPool;
        pool.erase(std::remove(pool.begin(), pool.end(), ev.id), pool.end());
    }

    choiceTurnHistory[choiceKey].push_back(turnCount);

    if (!turnStarted && currentEventId == "intro_23") {
        turnStarted = true;
        turnCount = 0;
    }
    if (turnStarted) {
        turnCount++;
    }

    // Outcomes 처리
    bool outcome_matched = false;
    if (!choice.outcomes.empty()) {
        for (const auto& outcome : choice.outcomes) {
            bool allMet = true;
            if (!outcome.conditions.empty()) {
                for (const auto& cond : outcome.conditions) {
                    if (!isConditionMet(cond, player, turnCount)) {
                        allMet = false;
                        break;
                    }
                }
            }

            if (allMet) {
                player.money += outcome.moneyDelta;
                player.hp += outcome.hpDelta;
                player.sanity += outcome.sanityDelta;
                player.strength += outcome.strengthDelta;
                player.hacking += outcome.hackingDelta;
                player.cloneBodies += outcome.cloneDelta;
                for (const auto& item : outcome.itemChanges) {
                    player.addItem(item.first, item.second);
                }
                for (const auto& info : outcome.infoChanges) {
                    player.addInformation(info.first, info.second);
                }

                if (outcome.nextEventId == "random" || outcome.nextEventId == "random_once") {
                    currentEventId = getNextEventId();
                    goingRandom = true;
                }
                else {
                    currentEventId = outcome.nextEventId;
                }
                outcome_matched = true;
                break;
            }
        }
    }

    if (!outcome_matched) {
        player.money += choice.moneyDelta;
        player.hp += choice.hpDelta;
        player.sanity += choice.sanityDelta;
        player.strength += choice.strengthDelta;
        player.hacking += choice.hackingDelta;
        player.cloneBodies += choice.cloneDelta;
        for (const auto& item : choice.itemGains) {
            player.addItem(item.first, item.second);
        }
        for (const auto& info : choice.informationGains) {
            player.addInformation(info.first, info.second);
        }

        if (choice.nextEventId == "random" || choice.nextEventId == "random_once") {
            currentEventId = getNextEventId();
            goingRandom = true;
        }
        else {
            currentEventId = choice.nextEventId;
        }
    }

    player.applyStatLimits();

    if (player.isDead()) {
        currentEventId = "death_notice";
        goingRandom = false;
    }

    if (goingRandom) {
        scenarioCount++;
    }
}

// getNextEventId, checkEventCondition, getPlayerStat, isConditionMet 함수는
// 이전과 동일하게 유지합니다.
std::string Game::getNextEventId() {
    if (!forceNextEventId.empty()) {
        std::string id = forceNextEventId;
        forceNextEventId.clear();
        return id;
    }

    std::vector<std::pair<std::string, Event*>> candidateForced;
    for (auto& item : eventManager.events) {
        auto& ev = item.second;
        if (ev.type == "forced" && checkEventCondition(ev)) {
            candidateForced.push_back({ item.first, &ev });
        }
    }

    if (!candidateForced.empty()) {
        for (auto& candidate : candidateForced) {
            if (candidate.first.find("death") != std::string::npos ||
                candidate.first.find("clone") != std::string::npos) {
                return candidate.first;
            }
        }
        return candidateForced[0].first;
    }

    if (eventManager.randomPool.empty()) {
        throw std::runtime_error("[FATAL] No random events available!");
    }

    std::vector<std::string> candidates;
    for (auto& id : eventManager.randomPool) {
        if (std::find(recentEvents.begin(), recentEvents.end(), id) == recentEvents.end()) {
            candidates.push_back(id);
        }
    }

    if (candidates.empty()) {
        recentEvents.clear();
        candidates = eventManager.randomPool;
    }

    int idx = rand() % candidates.size();
    std::string chosen = candidates[idx];

    recentEvents.push_back(chosen);
    if (recentEvents.size() > 5) recentEvents.pop_front();

    return chosen;
}

bool Game::checkEventCondition(const Event& ev) {
    // excludedIfInfoOwned 조건 검사: 플레이어가 해당 정보를 1개 이상 가지고 있으면 제외
    for (const auto& blockedInfo : ev.condition.excludedIfInfoOwned) {
        auto it = player.information.find(blockedInfo);
        if (it != player.information.end() && it->second > 0) {
            return false;
        }
    }

    // 최소 턴 수 조건
    if (ev.condition.minTurns > 0 && turnCount < ev.condition.minTurns) {
        return false;
    }

    // 최소 시나리오 진행 조건
    if (ev.condition.minScenarios > 0 && scenarioCount < ev.condition.minScenarios) {
        return false;
    }

    // 스탯 조건 검사
    for (const auto& [statName, statCond] : ev.condition.stats) {
        int playerValue = getPlayerStat(player, statName);
        if (playerValue < statCond.min || playerValue > statCond.max) {
            return false;
        }
    }

    // 특정 선택지(choices) 필수 여부 검사
    for (const auto& choiceKey : ev.condition.requiredChoices) {
        if (playerHistory.find(choiceKey) == playerHistory.end()) {
            return false;
        }
    }

    // 제외할 선택지 검사(이 부분은 만약 있으면 추가)
    for (const auto& blockedChoice : ev.condition.excludedIfChoiceMade) {
        if (playerHistory.find(blockedChoice) != playerHistory.end()) {
            return false;
        }
    }

    return true;
}

int Game::getPlayerStat(const Player& player, const std::string& statName) {
    if (statName == "strength") return player.strength;
    if (statName == "hacking") return player.hacking;
    if (statName == "hp") return player.hp;
    if (statName == "sanity") return player.sanity;
    if (statName == "money") return player.money;
    if (statName == "clones") return player.cloneBodies;
    return 0;
}

bool Game::isConditionMet(const Condition& cond, const Player& player, int turnCount) {
    bool hasAnyCondition = !cond.stat.empty() || !cond.item.empty() || !cond.info.empty() ||
        !cond.afterChoice.empty() || cond.hasTurn;
    if (!hasAnyCondition) {
        return true;
    }

    if (!cond.stat.empty()) {
        if (cond.stat == "random") {
            return (rand() % 100 + 1) <= cond.min;
        }
        return getPlayerStat(player, cond.stat) >= cond.min;
    }

    if (!cond.item.empty()) {
        auto it = player.items.find(cond.item);
        if (it == player.items.end()) return false;
        return it->second >= cond.min;
    }

    if (!cond.info.empty()) {
        auto it = player.information.find(cond.info);
        if (it == player.information.end()) return false;
        return it->second >= cond.min;
    }

    if (!cond.afterChoice.empty() && cond.turnsPassed > 0) {
        auto it = choiceTurnHistory.find(cond.afterChoice);
        if (it == choiceTurnHistory.end()) return false;
        for (int choiceTurn : it->second) {
            if (turnCount - choiceTurn >= cond.turnsPassed) {
                return true;
            }
        }
        return false;
    }

    if (cond.hasTurn) {
        return turnCount >= cond.turnValue;
    }

    return true;
}