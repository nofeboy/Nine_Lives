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
        ev.type = e.value("type", "random");   // ⭐⭐⭐ 이 줄을 꼭 추가! ⭐⭐⭐

        ev.id = e["id"];
        ev.description = e["description"];

        if (e.contains("condition")) {
            auto cond = e["condition"];
            ev.condition.minTurns = cond.value("minTurns", 0);
            // ⭐⭐⭐ 꼭 이 if문을 넣어! ⭐⭐⭐
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
                        // max는 INT_MAX 그대로
                    }
                    ev.condition.stats[item.key()] = sc;
                }
            }


            if (cond.contains("choicesMade")) {
                for (auto& ch : cond["choicesMade"]) {
                    ev.condition.requiredChoices.push_back(ch);
                }
            }
        }

        std::string type = e.value("type", "random");
        if (type == "random") {
            randomPool.push_back(ev.id);
        }
        else if (type == "forced") {
            std::string trigger = e.value("trigger", "");
            if (!trigger.empty()) {
                forcedEvents[trigger].push_back(ev.id);
            }
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

            // outcomes 파싱 추가
            if (c.contains("outcomes")) {
                for (auto& o : c["outcomes"]) {
                    Outcome out;
                    if (o.contains("conditions")) {
                        for (auto& cond : o["conditions"]) {
                            Condition cd;

                            if (cond.contains("excludedIfChoiceMade")) {
                                for (auto& ex : cond["excludedIfChoiceMade"]) {
                                    cd.excludedIfChoiceMade.push_back(ex);
                                }
                            }

                            if (cond.contains("afterChoice")) {
                                cd.afterChoice = cond["afterChoice"];
                            }
                            if (cond.contains("turnsPassed")) {
                                cd.turnsPassed = cond["turnsPassed"];
                            }

                            // stat 조건
                            if (cond.contains("stat")) {
                                cd.stat = cond["stat"];
                                cd.min = cond.value("min", 0);
                            }
                            // item 조건
                            if (cond.contains("item")) {
                                cd.item = cond["item"];
                                cd.min = cond.value("min", 1);
                            }
                            // info 조건
                            if (cond.contains("info")) {
                                cd.info = cond["info"];
                                cd.min = cond.value("min", 1);
                            }
                            // turn 조건
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

    std::cerr << "[DEBUG] Loaded events: " << events.size() << std::endl;
    std::cerr << "[DEBUG] Random pool size: " << randomPool.size() << std::endl;
}

Event& EventManager::getEvent(const std::string& id) {
    if (id == "random") {
        throw std::runtime_error("[FATAL] Attempted to fetch 'random' as event ID. Logic error!");
    }
    auto it = events.find(id);
    if (it == events.end()) {
        throw std::runtime_error("[FATAL] Event ID not found: " + id);
    }
    return it->second;
}

std::vector<std::string> EventManager::getForcedEvents(const std::string& trigger) {
    if (forcedEvents.count(trigger)) return forcedEvents[trigger];
    return {};
}
