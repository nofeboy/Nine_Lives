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
		forceNextEventId = eventManager.randomPool.front();
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

		if (ev.choices.empty()) {
			Renderer::renderEventFull(ev, player, animateCard);
			if (player.isCompletelyDead()) Renderer::showCompleteGameOver();
			else Renderer::showGameOver();
			break;
		}

		if (!previewMode) {
			Renderer::renderEventFull(ev, player, animateCard);
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

			bool locked = (player.strength < ev.choices[idx].reqStrength || player.hacking < ev.choices[idx].reqHacking);
			for (const string& reqItem : ev.choices[idx].requiredItems) {
				if (player.items.find(reqItem) == player.items.end() || player.items.at(reqItem) <= 0) {
					locked = true;
					break;
				}
			}

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

	// ✅ 선택 기록 추가
	playerHistory.insert(ev.id + ":" + choice.text);

	// ✅ 턴 증가
	turnCount++;

	// ✅ 기존 HP, sanity, money 변경 로직 유지
	player.hp += choice.hpDelta;
	player.sanity += choice.sanityDelta;
	player.money += choice.moneyDelta;
	player.strength += choice.strengthDelta;
	player.hacking += choice.hackingDelta;

	if (player.hp > 7) player.hp = 7;
	if (player.hp < 0) player.hp = 0;
	if (player.sanity > 7) player.sanity = 7;
	if (player.sanity < 0) player.sanity = 0;
	if (player.strength > 7) player.strength = 7;
	if (player.hacking > 7) player.hacking = 7;
	if (player.money < 0) player.money = 0;


	for (const auto& item : choice.itemGains) {
		player.addItem(item.first, item.second);
	}
	for (const auto& info : choice.informationGains) {
		player.addInformation(info.first, info.second);
	}

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

	for (auto& item : eventManager.events) {
		auto& ev = item.second;
		if (ev.type == "forced" && checkEventCondition(ev)) {
			// 모든 선택지가 단 하나라도 기록됐으면 무조건 넘어감
			bool alreadySeen = false;
			for (auto& ch : ev.choices) {
				if (playerHistory.count(ev.id + ":" + ch.text)) {
					alreadySeen = true;
					break;
				}
			}
			// **여기서** 이미 본 이벤트면 continue, 안 본 이벤트만 리턴
			if (alreadySeen) continue;
			return item.first;
		}
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


bool Game::checkEventCondition(const Event& ev) {
	if (ev.condition.minTurns > 0 && turnCount < ev.condition.minTurns) return false;

	for (const auto& item : ev.condition.stats) {
		const std::string& stat = item.first;
		int minVal = item.second.min;
		if (stat == "strength" && player.strength < minVal) return false;
		if (stat == "hacking" && player.hacking < minVal) return false;
		if (stat == "hp" && player.hp < minVal) return false;
		if (stat == "sanity" && player.sanity < minVal) return false;
		if (stat == "money" && player.money < minVal) return false;
	}

	for (auto& choiceKey : ev.condition.requiredChoices) {
		if (playerHistory.find(choiceKey) == playerHistory.end()) return false;
	}

	return true;
}
