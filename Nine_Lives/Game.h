#pragma once
#include <string>
#include "Player.h"
#include "EventManager.h"

class Game {
public:
    void run();
private:
    Player player;
    EventManager eventManager;
    std::string currentEventId = "intro_1";
    void processChoice(int choiceIndex);
    void setConsoleSize(int width, int height);
    void disableQuickEdit();
};
