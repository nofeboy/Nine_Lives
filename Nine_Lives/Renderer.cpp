#include "Renderer.h"
#include "ConsoleUtils.h"
#include "Game.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <conio.h>

#define RESET   u8"\033[0m"
#define CYAN    u8"\033[36m"
#define MAGENTA u8"\033[35m"
#define WHITE   u8"\033[97m"
#define GRAY    u8"\033[90m"
#define BOLD    u8"\033[1m"
#define YELLOW  u8"\033[33m"
#define RED     u8"\033[31m"
#define GREEN   u8"\033[32m"

using namespace std;

const int SCREEN_WIDTH = 120;
const int SCREEN_HEIGHT = 40;
const int CARD_WIDTH = 40;
const int CARD_HEIGHT = 15;

const int CARD_POS_X = (SCREEN_WIDTH - CARD_WIDTH) / 2;
const int CARD_POS_Y = 8;

static void sleep_ms(int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}

// ✅ 문자열 실제 출력 폭 계산 (한글 대응)
int getDisplayWidth(const string& text) {
    int width = 0;
    for (size_t i = 0; i < text.length();) {
        unsigned char c = text[i];
        if ((c & 0x80) == 0) { width += 1; i += 1; }
        else if ((c & 0xE0) == 0xC0) { width += 2; i += 2; }
        else if ((c & 0xF0) == 0xE0) { width += 2; i += 3; }
        else if ((c & 0xF8) == 0xF0) { width += 2; i += 4; }
        else { i++; }
    }
    return width;
}

//선택지 줄 자동 정리
vector<string> wrapChoiceText(const string& text, int maxWidth) {
    vector<string> lines;
    string currentLine;
    int currentWidth = 0;

    for (size_t i = 0; i < text.size();) {
        unsigned char c = text[i];
        int charWidth = 1, byteCount = 1;

        if ((c & 0x80) == 0) { charWidth = 1; byteCount = 1; }
        else if ((c & 0xE0) == 0xC0) { charWidth = 2; byteCount = 2; }
        else if ((c & 0xF0) == 0xE0) { charWidth = 2; byteCount = 3; }
        else if ((c & 0xF8) == 0xF0) { charWidth = 2; byteCount = 4; }

        if (currentWidth + charWidth > maxWidth) {
            lines.push_back(currentLine);
            currentLine.clear();
            currentWidth = 0;
        }

        currentLine += text.substr(i, byteCount);
        currentWidth += charWidth;
        i += byteCount;
    }

    if (!currentLine.empty()) lines.push_back(currentLine);
    return lines;
}

//선택지 출력
static void drawChoiceBlock(int index, const string& text, bool locked, bool highlight,
    int cardCenterX, int cardBottomY, int leftX, int rightX) {
    auto wrapped = wrapChoiceText(text, 32);
    string arrow;
    int baseY, baseX;

    switch (index) {
    case 0: // RIGHT
        arrow = "[RIGHT] ";
        baseY = CARD_POS_Y + CARD_HEIGHT / 2;
        baseX = rightX - 3;
        break;

    case 1: { // LEFT (맞춤 계산)
        arrow = "[LEFT] ";
        baseY = CARD_POS_Y + CARD_HEIGHT / 2;
        int textWidth = getDisplayWidth(wrapped[0]);
        baseX = (CARD_POS_X - 2) - ((int)arrow.size() + textWidth);

        // 👉 왼쪽 경계 침범 방지
        if (baseX < 2) baseX = 2;
        break;
    }

    case 2: // UP
        arrow = "[UP] ";
        baseY = CARD_POS_Y - 3;
        baseX = cardCenterX - (int)arrow.size() / 2 - 16;
        break;

    case 3: // DOWN
        arrow = "[DOWN] ";
        baseY = cardBottomY + 3;
        baseX = cardCenterX - (int)arrow.size() / 2 - 16;
        break;
    }

    // ✅ 색상 결정
    string color = locked ? GRAY : (highlight ? YELLOW : MAGENTA);

    // ✅ 첫 줄
    cout << "\033[" << baseY << ";" << baseX << "H" << color << arrow << wrapped[0] << RESET;

    // ✅ 두 번째 줄
    if (wrapped.size() > 1) {
        cout << "\033[" << (baseY + 1) << ";" << (baseX + (int)arrow.size()) << "H" << color << wrapped[1] << RESET;
    }

    // ✅ LOCKED 라벨
    if (locked) {
        cout << "\033[" << (baseY + (int)wrapped.size()) << ";" << (baseX + (int)arrow.size())
            << "H" << GRAY << "[LOCKED]" << RESET;
    }
}

// ✅ 카드 내부 줄바꿈 처리 + \n 대응
vector<string> wrapText(const string& text) {
    vector<string> lines;
    string currentLine = "";
    int currentWidth = 0;
    const int maxWidth = CARD_WIDTH - 4;

    for (size_t i = 0; i < text.length();) {
        if (text[i] == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
            currentWidth = 0;
            i++;
            continue;
        }

        unsigned char c = text[i];
        int charWidth = 1, byteCount = 1;

        if ((c & 0x80) == 0) { charWidth = 1; byteCount = 1; }
        else if ((c & 0xE0) == 0xC0) { charWidth = 2; byteCount = 2; }
        else if ((c & 0xF0) == 0xE0) { charWidth = 2; byteCount = 3; }
        else if ((c & 0xF8) == 0xF0) { charWidth = 2; byteCount = 4; }
        else { i++; continue; }

        if (currentWidth + charWidth > maxWidth) {
            lines.push_back(currentLine);
            currentLine.clear();
            currentWidth = 0;
        }

        currentLine += text.substr(i, byteCount);
        currentWidth += charWidth;
        i += byteCount;
    }

    if (!currentLine.empty()) lines.push_back(currentLine);
    return lines;
}

// ✅ 카드 즉시 출력
void drawCardInstant(const string& text) {
    auto lines = wrapText(text);

    // 카드 상단
    cout << "\033[" << CARD_POS_Y << ";" << CARD_POS_X << "H"
        << CYAN << "+" << string(CARD_WIDTH - 2, '-') << "+" << RESET;

    for (int row = 0; row < CARD_HEIGHT - 2; row++) {
        cout << "\033[" << (CARD_POS_Y + 1 + row) << ";" << CARD_POS_X << "H"
            << CYAN << "|" << RESET;

        // **텍스트 커서 위치 지정**
        cout << "\033[" << (CARD_POS_Y + 1 + row) << ";" << (CARD_POS_X + 1) << "H";

        if (row < (int)lines.size()) {
            string line = lines[row];
            int displayWidth = getDisplayWidth(line);
            cout << line << string(CARD_WIDTH - 2 - displayWidth, ' ');
        }
        else {
            cout << string(CARD_WIDTH - 2, ' ');
        }

        cout << CYAN << "|" << RESET;
    }

    cout << "\033[" << (CARD_POS_Y + CARD_HEIGHT - 1) << ";" << CARD_POS_X << "H"
        << CYAN << "+" << string(CARD_WIDTH - 2, '-') << "+" << RESET;
    cout.flush();
}

// ✅ 카드 애니메이션 출력
void drawCardAnimated(const std::string& text) {
    auto lines = wrapText(text);
    bool skipAll = false;

    // 카드 프레임 출력
    cout << "\033[" << CARD_POS_Y << ";" << CARD_POS_X << "H"
        << CYAN << "+" << string(CARD_WIDTH - 2, '-') << "+" << RESET;

    for (int row = 0; row < CARD_HEIGHT - 2; row++) {
        cout << "\033[" << (CARD_POS_Y + 1 + row) << ";" << CARD_POS_X << "H"
            << CYAN << "|" << RESET << string(CARD_WIDTH - 2, ' ') << CYAN << "|" << RESET;
    }

    cout << "\033[" << (CARD_POS_Y + CARD_HEIGHT - 1) << ";" << CARD_POS_X << "H"
        << CYAN << "+" << string(CARD_WIDTH - 2, '-') << "+" << RESET;
    cout.flush();

    // 내부 텍스트 애니메이션 출력
    for (int row = 0; row < CARD_HEIGHT - 2; row++) {
        if (row >= (int)lines.size()) break;
        const string& line = lines[row];

        size_t i = 0;
        int cursorX = CARD_POS_X + 1; // 텍스트 시작 위치
        while (i < line.size()) {
            // UTF-8 문자 추출
            unsigned char c = line[i];
            int byteCount = 1;
            int charWidth = 1;

            if ((c & 0x80) == 0) { byteCount = 1; charWidth = 1; }
            else if ((c & 0xE0) == 0xC0) { byteCount = 2; charWidth = 2; }
            else if ((c & 0xF0) == 0xE0) { byteCount = 3; charWidth = 2; }
            else if ((c & 0xF8) == 0xF0) { byteCount = 4; charWidth = 2; }

            string utf8Char = line.substr(i, byteCount);

            // 커서 이동 후 출력
            cout << "\033[" << (CARD_POS_Y + 1 + row) << ";" << cursorX << "H" << utf8Char << flush;

            if (!skipAll) {
                if (_kbhit()) {
                    _getch(); // 아무 키 누르면 skipAll 활성화
                    skipAll = true;
                }
                else {
                    this_thread::sleep_for(chrono::milliseconds(15)); // 속도 조절
                }
            }

            cursorX += charWidth;
            i += byteCount;

            if (skipAll) {
                // 남은 부분 한 번에 출력
                while (i < line.size()) {
                    c = line[i];
                    if ((c & 0x80) == 0) { byteCount = 1; charWidth = 1; }
                    else if ((c & 0xE0) == 0xC0) { byteCount = 2; charWidth = 2; }
                    else if ((c & 0xF0) == 0xE0) { byteCount = 3; charWidth = 2; }
                    else if ((c & 0xF8) == 0xF0) { byteCount = 4; charWidth = 2; }
                    utf8Char = line.substr(i, byteCount);

                    cout << "\033[" << (CARD_POS_Y + 1 + row) << ";" << cursorX << "H" << utf8Char;
                    cursorX += charWidth;
                    i += byteCount;
                }
                cout.flush();
            }
        }
    }
}

// ✅ 카드 내부 텍스트만 삭제
void Renderer::eraseCardText(const string&) {
    for (int row = 0; row < CARD_HEIGHT - 2; row++) {
        cout << "\033[" << (CARD_POS_Y + 1 + row) << ";" << (CARD_POS_X + 1) << "H"
            << string(CARD_WIDTH - 2, ' ') << flush;
    }
    sleep_ms(100);
}

// ✅ 화면 전체 렌더 (HUD + 카드 + 선택지 + 아이템)
void Renderer::renderEventFull(const Event& ev, const Player& player, bool animateCard, int turnCount, int scenarioCount) {
    ConsoleUtils::clearScreen();
    drawHUD(player, turnCount, scenarioCount);

    if (animateCard) {
        drawCardAnimated(ev.description); // 카드 애니메이션 끝나고
    }
    else {
        drawCardInstant(ev.description);
    }

    // ✅ 여기서 선택지 출력
    renderChoices(ev, player, NONE);
    cout.flush();

    // ✅ 아이템 출력
    int cardBottomY = CARD_POS_Y + CARD_HEIGHT;
    cout << "\033[" << (cardBottomY + 6) << ";1H";
    drawItems(player);
}

// ✅ 선택지 강조만 업데이트
void Renderer::updateChoicesOnly(const Event& ev, const Player& player, Direction previewDir) {
    int cardCenterX = CARD_POS_X + CARD_WIDTH / 2;
    int cardBottomY = CARD_POS_Y + CARD_HEIGHT;
    int leftX = CARD_POS_X - 30;
    int rightX = CARD_POS_X + CARD_WIDTH + 4;

    for (size_t i = 0; i < ev.choices.size(); i++) {
        bool locked = Game::isChoiceLocked(ev.choices[i], player); // ✅ Game:: 사용
        bool highlight = (previewDir == RIGHT && i == 0) || (previewDir == LEFT && i == 1) ||
            (previewDir == UP && i == 2) || (previewDir == DOWN && i == 3);
        drawChoiceBlock((int)i, ev.choices[i].text, locked, highlight, cardCenterX, cardBottomY, leftX, rightX);
    }
    cout.flush();
}

// ✅ HUD
void Renderer::drawHUD(const Player& player, int turnCount, int scenarioCount) {
    cout << "\n" << CYAN << BOLD << "[ STATS ]" << RESET
        << " HP: " << player.hp
        << " | SAN: " << player.sanity
        << " | STR: " << player.strength
        << " | HACK: " << player.hacking
        << " | money: " << player.money 
        << " | " << GREEN << "CLONES: " << player.cloneBodies << "/9" << RESET << "\n";
    if (turnCount > 0) {
        cout << " | " << YELLOW << "TURN: " << turnCount << RESET;
    }
    if (scenarioCount > 0) {
        cout << " | " << YELLOW << "SN: " << scenarioCount << RESET;
    }
    cout << "\n";
}

// ✅ 아이템 표시
void Renderer::drawItems(const Player& player) {
    // 아이템 라인
    cout << MAGENTA << "ITEM: " << RESET;
    if (player.items.empty()) {
        cout << u8"(없음)";
    }
    else {
        for (auto& it : player.items) {
            cout << it.first;
            if (it.second > 1) cout << "*" << it.second;
            cout << "  ";
        }
    }
    cout << "\n\n";

    // 정보 라인
    cout << CYAN << "INFO: " << RESET;
    if (player.information.empty()) {
        cout << u8"(없음)";
    }
    else {
        for (auto& info : player.information) {
            cout << info.first;
            if (info.second > 1) cout << "*" << info.second;
            cout << "  ";
        }
    }
    cout << "\n";
}

// ✅ 경고
void Renderer::showLockedWarning() {
    for (int i = 0; i < 2; i++) {
        cout << "\n" << RED << u8"선택 조건 부족" << RESET << flush;
        sleep_ms(200);
        cout << "\r" << string(30, ' ') << "\r" << flush;
        sleep_ms(200);
    }
}

void Renderer::showCloneRevival() {
    ConsoleUtils::clearScreen();
    cout << "\n\n" << GREEN << BOLD;
    cout << "    +-------+  +-------+  +-------+  +---------+  +-------+     +-------+  +-------+ \n";
    cout << "    | CLONE |  | BODY  |  | READY |  | REVIVAL |  | START |     | AGAIN |  | READY | \n";
    cout << "    +-------+  +-------+  +-------+  +---------+  +-------+     +-------+  +-------+ \n";
    cout << RESET << "\n\n" << YELLOW << u8"클론 바디 활성화... 의식 전송 중..." << RESET << "\n\n";

    for (int i = 0; i < 5; i++) {
        cout << "#" << flush;
        sleep_ms(500);
    }
    cout << "\n\n" << GREEN << u8"부활 완료! 새로운 클론 바디로 깨어났습니다." << RESET << "\n";
    sleep_ms(2000);
}

void Renderer::showGameOver() {
    ConsoleUtils::clearScreen();
    const string art = u8R"(
   +-------+  +-------+  +-------+  +-------+     +-------+  +-------+  +-------+  +-------+
   | GAME  |  | OVER  |  | THIS  |  | LIFE  |     | ENDS  |  | HERE  |  | BUT   |  | CLONE |
   +-------+  +-------+  +-------+  +-------+     +-------+  +-------+  +-------+  +-------+
)";
    cout << MAGENTA << art << RESET << endl;
    cout << "\n" << RED << u8"이번 생은 여기까지..." << RESET << "\n\n";
    cout << CYAN << u8"아무 키나 눌러서 계속하세요." << RESET << "\n";
    cin.get();
}

void Renderer::showCompleteGameOver() {
    ConsoleUtils::clearScreen();
    const string deathArt = u8R"(
  +-------+  +-------+  +-------+     +-------+  +-------+  +-------+
  |  THE  |  | FINAL |  |  END  |     | DEATH |  | COMES |  |  ALL  |
  +-------+  +-------+  +-------+     +-------+  +-------+  +-------+
)";

    for (int i = 0; i < 3; i++) {
        cout << RED << deathArt << RESET << endl;
        sleep_ms(500);
        ConsoleUtils::clearScreen();
        sleep_ms(300);
    }

    cout << RED << deathArt << RESET << endl;
    cout << "\n" << RED << BOLD << u8"모든 클론 바디 소진... 완전한 죽음..." << RESET << "\n\n";
    cout << GRAY << u8"복수는 실패로 끝났다..." << RESET << "\n\n";
    cout << CYAN << u8"아무 키나 눌러서 종료하세요." << RESET << "\n";
    cin.get();
}

void Renderer::renderChoices(const Event& ev, const Player& player, Direction) {
    int cardCenterX = CARD_POS_X + CARD_WIDTH / 2;
    int cardBottomY = CARD_POS_Y + CARD_HEIGHT;
    int leftX = CARD_POS_X - 30;
    int rightX = CARD_POS_X + CARD_WIDTH + 4;

    for (size_t i = 0; i < ev.choices.size(); i++) {
        bool locked = Game::isChoiceLocked(ev.choices[i], player); // ✅ Game:: 사용
        drawChoiceBlock((int)i, ev.choices[i].text, locked, false, cardCenterX, cardBottomY, leftX, rightX);
    }
    cout.flush();
}

void Renderer::showReviveAnimation() {
    ConsoleUtils::clearScreen();
    std::string ascii =
        "\n\n   ████  █    █ ██████  ██████\n"
        "   █     █    █ █    █  █    █\n"
        "   ████  █    █ ██████  █    █\n"
        "   █     █    █ █   █   █    █\n"
        "   █     ██████ █    █  ██████\n"
        "\n[클론 바디 재부팅 중...]\n";
    for (char c : ascii) {
        std::cout << c << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1200)); // 잠시 정지
    ConsoleUtils::clearScreen();
}
