#include "EventManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

void EventManager::loadEvents(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("[FATAL] Cannot open events.json: " + filename);
    }

    json data;
    file >> data;

    for (auto& e : data["events"]) {
        Event ev;
        ev.type = e.value("type", "random");
        ev.id = e["id"];
        ev.description = e["description"];

        if (e.contains("condition")) {
            auto cond = e["condition"];
            ev.condition.minTurns = cond.value("minTurns", 0);
            ev.condition.minScenarios = cond.value("minScenarios", 0);


            if (cond.contains("afterChoice")) {
                ev.condition.afterChoice = cond["afterChoice"];
            }
            if (cond.contains("turnsPassed")) {
                ev.condition.turnsPassed = cond["turnsPassed"];
            }

            if (cond.contains("stats") && !cond["stats"].is_null()) {
                for (auto& item : cond["stats"].items()) {
                    StatCondition sc;
                    if (item.value().is_object()) {
                        if (item.value().contains("min") && item.value()["min"].is_number_integer())
                            sc.min = item.value()["min"].get<int>();
                        if (item.value().contains("max") && item.value()["max"].is_number_integer())
                            sc.max = item.value()["max"].get<int>();
                    }
                    else if (item.value().is_number_integer()) {
                        sc.min = item.value().get<int>();
                    }
                    ev.condition.stats[item.key()] = sc;
                }
            }

            if (cond.contains("choicesMade")) {
                for (auto& ch : cond["choicesMade"]) {
                    ev.condition.requiredChoices.push_back(ch);
                }
            }
            if (cond.contains("excludedIfChoiceMade")) { // ★★★★★ 추가
                for (auto& ch : cond["excludedIfChoiceMade"]) {
                    ev.condition.excludedIfChoiceMade.push_back(ch);
                }
            }
            if (cond.contains("excludedIfInfoOwned")) {
                for (auto& info : cond["excludedIfInfoOwned"]) {
                    ev.condition.excludedIfInfoOwned.push_back(info);
                }
            }
        }

        std::string type = e.value("type", "random");
        if (type == "random" || type == "random_once") {
            randomPool.push_back(ev.id);
        }
        else if (type == "forced") {
            // "forced" 타입 이벤트는 별도로 처리하지 않아도 getNextEventId에서 전체를 순회
        }

        for (auto& c : e["choices"]) {
            Choice ch;
            ch.text = c["text"];
            ch.reqStrength = c.value("reqStrength", 0);
            ch.reqHacking = c.value("reqHacking", 0);
            ch.hpDelta = c.value("hpDelta", 0);
            ch.sanityDelta = c.value("sanityDelta", 0);
            ch.moneyDelta = c.value("moneyDelta", 0);
            ch.nextEventId = c["nextEventId"];
            ch.strengthDelta = c.value("strengthDelta", 0);
            ch.hackingDelta = c.value("hackingDelta", 0);
            ch.cloneDelta = c.value("cloneDelta", 0);

            if (c.contains("outcomes")) {
                for (auto& o : c["outcomes"]) {
                    Outcome out;
                    if (o.contains("conditions")) {
                        for (auto& cond : o["conditions"]) {
                            Condition cd;

                            // ... (다른 조건 파싱) ...
                            if (cond.contains("excludedIfChoiceMade")) {
                                for (auto& ex : cond["excludedIfChoiceMade"]) {
                                    cd.excludedIfChoiceMade.push_back(ex);
                                }
                            }

                            // ★★★★★ 여기가 수정된 부분입니다. (outcome 내부의 조건) ★★★★★
                            if (cond.contains("excludedIfInfoOwned")) {
                                for (auto& info : cond["excludedIfInfoOwned"]) {
                                    cd.excludedIfInfoOwned.push_back(info); // ev -> cd 로 수정
                                }
                            }

                            if (cond.contains("afterChoice")) {
                                cd.afterChoice = cond["afterChoice"];
                            }
                            if (cond.contains("turnsPassed")) {
                                cd.turnsPassed = cond["turnsPassed"];
                            }
                            if (cond.contains("stat")) {
                                cd.stat = cond["stat"];
                                cd.min = cond.value("min", 0);
                            }
                            if (cond.contains("item")) {
                                cd.item = cond["item"];
                                cd.min = cond.value("min", 1);
                            }
                            if (cond.contains("info")) {
                                cd.info = cond["info"];
                                cd.min = cond.value("min", 1);
                            }
                            if (cond.contains("turn")) {
                                cd.hasTurn = true;
                                cd.turnValue = cond["turn"];
                            }
                            out.conditions.push_back(cd);
                        }
                    }

                    out.moneyDelta = o.value("moneyDelta", 0);
                    out.hpDelta = o.value("hpDelta", 0);
                    out.sanityDelta = o.value("sanityDelta", 0);
                    out.strengthDelta = o.value("strengthDelta", 0);
                    out.hackingDelta = o.value("hackingDelta", 0);
                    out.cloneDelta = o.value("cloneDelta", 0);

                    if (o.contains("itemChanges")) {
                        for (auto& item : o["itemChanges"].items()) {
                            out.itemChanges[item.key()] = item.value();
                        }
                    }

                    if (o.contains("infoChanges")) {
                        for (auto& info : o["infoChanges"].items()) {
                            out.infoChanges[info.key()] = info.value();
                        }
                    }

                    out.nextEventId = o["nextEventId"];
                    ch.outcomes.push_back(out);
                }
            }

            if (c.contains("itemGains")) {
                for (auto& item : c["itemGains"]) {
                    ch.itemGains.push_back({ item[0], item[1] });
                }
            }

            if (c.contains("informationGains")) {
                for (auto& info : c["informationGains"]) {
                    ch.informationGains.push_back({ info[0], info[1] });
                }
            }

            if (c.contains("requiredItems")) {
                for (auto& req : c["requiredItems"]) {
                    ch.requiredItems.push_back(req);
                }
            }

            ev.choices.push_back(ch);
        }

        events[ev.id] = ev;
    }

}


Event& EventManager::getEvent(const std::string& id) {
    if (id == "random" || id == "random_once") {
        throw std::runtime_error("[FATAL] Attempted to fetch 'random' as event ID. Logic error!");
    }
    auto it = events.find(id);
    if (it == events.end()) {
        throw std::runtime_error("[FATAL] Event ID not found: " + id);
    }
    return it->second;
}

std::vector<std::string> EventManager::getForcedEvents(const std::string& trigger) {
    if (EventManager::forcedEvents.count(trigger)) return forcedEvents[trigger];
    return {};
}
