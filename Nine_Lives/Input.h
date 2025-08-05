#pragma once

enum Direction { NONE, UP, DOWN, LEFT, RIGHT };

class Input {
public:
    static Direction getDirection();
};
