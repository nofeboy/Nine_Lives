#include "ConsoleUtils.h"
#include <cstdlib>
#include <iostream>

void ConsoleUtils::setUTF8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

void ConsoleUtils::setConsoleSize(int width, int height) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT DisplayArea = { 0, 0, (short)(width - 1), (short)(height - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);
    COORD bufferSize = { (short)width, (short)height };
    SetConsoleScreenBufferSize(hOut, bufferSize);
}

void ConsoleUtils::enableANSI() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
}

void ConsoleUtils::clearScreen() {
    std::cout << "\033[2J\033[H" << std::flush; // ANSI 방식 클리어
}

void ConsoleUtils::disableResize() {
    HWND consoleWindow = GetConsoleWindow();
    LONG style = GetWindowLong(consoleWindow, GWL_STYLE);
    style &= ~WS_SIZEBOX;       // 크기 조정 비활성화
    style &= ~WS_MAXIMIZEBOX;   // 최대화 버튼 비활성화
    SetWindowLong(consoleWindow, GWL_STYLE, style);
}

void ConsoleUtils::hideCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;  // 커서 숨김
    SetConsoleCursorInfo(hOut, &cursorInfo);
}