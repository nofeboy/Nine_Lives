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

using namespace std;

// ✅ 선택지 잠금 체크 함수 추가 (Renderer에서도 사용)
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

	if (eventManager.events.empty()) {
		throw std::runtime_error("[FATAL] No events loaded from events.json!");
	}

	// ✅ 첫 이벤트 강제
	auto startEvents = eventManager.getForcedEvents("start");
	if (!startEvents.empty()) {
		forceNextEventId = startEvents.front();
	}
	else if (!eventManager.randomPool.empty()) {
		int idx = rand() % eventManager.randomPool.size();
		forceNextEventId = eventManager.randomPool[idx];
	}
	else {
		throw std::runtime_error("[FATAL] No start or random events available!");
	}

	Direction lastDir = NONE;
	bool previewMode = false;
	bool animateCard = true;

	while (true) {
		if (currentEventId.empty()) {
			currentEventId = getNextEventId();
			if (eventManager.events.find(currentEventId) == eventManager.events.end()) {
				throw std::runtime_error("[FATAL] Invalid event ID during loop: " + currentEventId);
			}
		}

		auto& ev = eventManager.getEvent(currentEventId);

		

		if (currentEventId == "gameover_screen") {
			Renderer::showGameOver();

			// ✅ 클론 부활 로직 개선
			if (player.cloneBodies > 0) {
				player.useCloneBody(); // 클론 사용 및 아이템 드랍 처리

				//// 드랍된 아이템이 있으면 알림
				//if (!player.lastDroppedItem.empty()) {
				//	Renderer::showItemDrop(player.lastDroppedItem);
				//}

				Renderer::showCloneRevival();
				currentEventId = "revive_story";
			}
			else {
				currentEventId = "death_final";
			}
			continue;
		}

		if (ev.choices.empty()) {
			Renderer::renderEventFull(ev, player, animateCard, turnCount);
			if (player.isCompletelyDead()) Renderer::showCompleteGameOver();
			else Renderer::showGameOver();
			break;
		}

		if (!previewMode) {
			Renderer::renderEventFull(ev, player, animateCard, turnCount);
			animateCard = false;
		}
		else {
			Renderer::updateChoicesOnly(ev, player, lastDir);
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

			// ✅ 잠금 체크를 함수로 통일
			bool locked = isChoiceLocked(ev.choices[idx], player);

			if (locked) {
				Renderer::showLockedWarning();
				previewMode = false;
				continue;
			}

			Renderer::eraseCardText(ev.description);
			processChoice(idx);
			previewMode = false;
			animateCard = true;

			continue;  // ✅ 이 줄 추가 → 다음 루프로 바로 넘어가서 새로운 이벤트를 로드
		}

		else {
			lastDir = dir;
		}
	}
}

void Game::processChoice(int choiceIndex) {
	auto& ev = eventManager.getEvent(currentEventId);
	auto& choice = ev.choices[choiceIndex];

	std::string choiceKey = ev.id + ":" + choice.text;
	playerHistory.insert(choiceKey);
	choiceTurnHistory[choiceKey].push_back(turnCount);

	if (!turnStarted && currentEventId == "intro_23") {
		turnStarted = true;
		turnCount = 0;
	}
	if (turnStarted) {
		turnCount++;
	}

	// ===== outcomes 분기 및 델타 적용 =====
	bool matched = false;
	if (!choice.outcomes.empty()) {
		for (const auto& outcome : choice.outcomes) {
			bool allMet = true;
			for (const auto& cond : outcome.conditions) {
				if (!isConditionMet(cond, player, turnCount)) {
					allMet = false;
					break;
				}
			}
			if (allMet) {
				// ✅ outcome의 델타 적용
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

				player.applyStatLimits();
				currentEventId = outcome.nextEventId;
				return;
			}
		}
	}

	// ✅ 조건 만족한 outcome 없으면 choice의 델타 적용
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

	player.applyStatLimits();

	if (choice.nextEventId == "random") {
		currentEventId = "";
	}
	else {
		currentEventId = choice.nextEventId;
	}
}

std::string Game::getNextEventId() {
	if (!forceNextEventId.empty()) {
		std::string id = forceNextEventId;
		forceNextEventId.clear();
		return id;
	}

	// ✅ 강제 이벤트 체크 (우선순위별로 정렬)
	std::vector<std::pair<std::string, Event*>> candidateForced;
	for (auto& item : eventManager.events) {
		auto& ev = item.second;
		if (ev.type == "forced" && checkEventCondition(ev)) {
			candidateForced.push_back({ item.first, &ev });
		}
	}

	// ✅ 강제 이벤트가 있다면 우선순위에 따라 선택
	if (!candidateForced.empty()) {
		// death 관련 이벤트 우선
		for (auto& candidate : candidateForced) {
			if (candidate.first.find("death") != std::string::npos ||
				candidate.first.find("clone") != std::string::npos) {
				return candidate.first;
			}
		}
		// 그 외는 첫 번째
		return candidateForced[0].first;
	}

	// ✅ 랜덤 이벤트 기존 로직
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

// ✅ 완전히 재작성된 checkEventCondition 함수
bool Game::checkEventCondition(const Event& ev) {

	for (const auto& blocked : ev.condition.excludedIfChoiceMade) {
		if (playerHistory.count(blocked)) return false;
	}

	// 턴 수 체크
	if (ev.condition.minTurns > 0 && turnCount < ev.condition.minTurns) {
		return false;
	}


	// 스탯 조건 체크
	for (const auto& item : ev.condition.stats) {
		const std::string& statName = item.first;
		const StatCondition& statCond = item.second;

		int playerValue = getPlayerStat(player, statName);

		// min/max 체크
		if (playerValue < statCond.min || playerValue > statCond.max) {
			return false;
		}
	}

	// 필요한 선택지 기록 체크
	for (const auto& choiceKey : ev.condition.requiredChoices) {
		if (playerHistory.find(choiceKey) == playerHistory.end()) {
			return false;
		}
	}

	return true;
}

// ✅ 플레이어 스탯 가져오기 함수 개선
int Game::getPlayerStat(const Player& player, const std::string& statName) {
	if (statName == "strength") return player.strength;
	if (statName == "hacking") return player.hacking;
	if (statName == "hp") return player.hp;
	if (statName == "sanity") return player.sanity;
	if (statName == "money") return player.money;
	if (statName == "clones") return player.cloneBodies;  // ✅ 추가
	return 0;
}

// ✅ outcomes 조건 체크 함수 개선
bool Game::isConditionMet(const Condition& cond, const Player& player, int turnCount) {
	// 스탯 조건
	if (!cond.stat.empty()) {
		// ✅ "random" 스탯은 확률로 처리
		if (cond.stat == "random") {
			int randomValue = rand() % 100 + 1; // 1~100
			return randomValue <= cond.min; // min 값이 확률(%)
		}
		return getPlayerStat(player, cond.stat) >= cond.min;
	}

	// 아이템 조건
	if (!cond.item.empty()) {
		auto it = player.items.find(cond.item);
		if (it == player.items.end()) return false;
		return it->second >= cond.min;
	}

	// 정보 조건
	if (!cond.info.empty()) {
		auto it = player.information.find(cond.info);
		if (it == player.information.end()) return false;
		return it->second >= cond.min;
	}

	// 후속 선택지 조건
	if (!cond.afterChoice.empty() && cond.turnsPassed > 0) {
		auto it = choiceTurnHistory.find(cond.afterChoice);
		if (it == choiceTurnHistory.end()) return false;

		bool met = false;
		for (int choiceTurn : it->second) {
			if (turnCount - choiceTurn >= cond.turnsPassed) {
				met = true;
				break;
			}
		}
		if (!met) return false;
	}


	// 턴 조건
	if (cond.hasTurn) {
		return turnCount >= cond.turnValue;
	}



	return false;
}