#include "Input.h"
#include <conio.h>

Direction Input::getDirection() {
    int ch = _getch();
    if (ch == 224) {
        int dir = _getch();
        switch (dir) {
        case 72: return UP;
        case 80: return DOWN;
        case 75: return LEFT;
        case 77: return RIGHT;
        }
    }
    return NONE;
}
