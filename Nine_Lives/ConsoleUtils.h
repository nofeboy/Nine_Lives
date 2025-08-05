#pragma once
#include <windows.h>
#include <string>

class ConsoleUtils {
public:
    static void setUTF8();
    static void setConsoleSize(int width, int height);
    static void clearScreen();    
    static void enableANSI();
    static void disableResize();
    static void hideCursor();
};
