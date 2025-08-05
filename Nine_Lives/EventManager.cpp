#include "EventManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;

void EventManager::loadEvents(const std::string& filename) {
    std::ifstream file(filename);
    json data;
    file >> data;

    for (auto& e : data["events"]) {
        Event ev;
        ev.id = e["id"];
        ev.description = e["description"];

        for (auto& c : e["choices"]) {
            Choice ch;
            ch.text = c["text"];
            ch.reqStrength = c["reqStrength"];
            ch.reqHacking = c["reqHacking"];
            ch.hpDelta = c["hpDelta"];
            ch.sanityDelta = c["sanityDelta"];
            ch.moneyDelta = c["moneyDelta"];
            ch.nextEventId = c["nextEventId"];

            // 아이템 획득 처리
            if (c.contains("itemGains")) {
                for (auto& item : c["itemGains"]) {
                    ch.itemGains.push_back({ item[0], item[1] });
                }
            }

            // 정보 획득 처리
            if (c.contains("informationGains")) {
                for (auto& info : c["informationGains"]) {
                    ch.informationGains.push_back({ info[0], info[1] });
                }
            }

            // 필요 아이템 처리
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
    return events.at(id);
}