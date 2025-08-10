#include "Renderer.h"
#include "ConsoleUtils.h"
#include "Game.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <conio.h>
#include <sstream>
#include <algorithm>   
#include <random>      

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


// ✅ 화면 중앙에 텍스트를 출력하는 헬퍼 함수
void printCentered(const std::string& text, int y_offset) {
    int textWidth = getDisplayWidth(text);
    int padding = (SCREEN_WIDTH - textWidth) / 2;
    if (padding < 0) padding = 0;

    // y_offset을 사용하여 특정 라인에 출력
    std::cout << "\033[" << y_offset << ";1H";
    std::cout << std::string(padding, ' ') << text << std::endl;
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
        baseY = CARD_POS_Y - 4;
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
    using namespace std::chrono;

    // ===== 콘솔 120x40 & ANSI 설정 =====
    system("mode con cols=120 lines=40");
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD outMode = 0; GetConsoleMode(hOut, &outMode);
    SetConsoleMode(hOut, outMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

//#ifndef BOLD
//#  define BOLD "\x1b[1m"
//#endif
//#ifndef GREEN
//#  define GREEN "\x1b[38;5;46m"
//#endif
//#ifndef RESET
//#  define RESET "\x1b[0m"
//#endif
#ifndef DIM
#  define DIM "\x1b[2m"
#endif

    // 커서 숨김
    std::cout << "\x1b[?25l";
    ConsoleUtils::clearScreen();

    // ===== 1) 타겟 ASCII =====
    std::string ascii = R"(

.########..########.##.....##.####.##.....##.####.##....##..######..
.##.....##.##.......##.....##..##..##.....##..##..###...##.##....##.
.##.....##.##.......##.....##..##..##.....##..##..####..##.##.......
.########..######...##.....##..##..##.....##..##..##.##.##.##...####.
.##...##...##........##...##...##...##...##...##..##..####.##....##.
.##....##..##.........##.##....##....##.##....##..##...###.##....##.
.##.....##.########....###....####....###....####.##....##..######..

                     [ Reviving clone body... ]
)";

    // ===== 2) 라인 분리 & 중앙 오프셋 =====
    std::vector<std::string> lines;
    {
        std::stringstream ss(ascii);
        std::string line;
        while (std::getline(ss, line)) lines.push_back(line);
    }
    size_t maxLen = 0;
    for (auto& l : lines) maxLen = max(maxLen, l.size());

    const int WIDTH = 120;
    const int HEIGHT = 40;

    int offsetX = max(0, (WIDTH - (int)maxLen) / 2);
    int offsetY = max(0, (HEIGHT - (int)lines.size()) / 2);

    // ===== 3) 매트릭스 레인: '세로 줄기' 상태 =====
    std::mt19937 rng((unsigned)high_resolution_clock::now().time_since_epoch().count());

    auto randChar = [&]() -> char {
        static const char table[] = "001101100111000111010101010"
            "23456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        std::uniform_int_distribution<int> d(0, (int)sizeof(table) - 2);
        return table[d(rng)];
        };

    struct Column {
        int x;
        int headY;
        int speed;      // 1~2 (속도)
        int trail;      // 2~3 (잔상 길이, 최대 3)
        int gap;        // 0~12 (다음 줄기 간격)
        int cool;       // 현재 쿨타임
        bool active;    // 줄기 진행 여부
    };

    std::vector<Column> cols(WIDTH);
    std::uniform_int_distribution<int> spd(3, 4);
    std::uniform_int_distribution<int> trl(2, 3);
    std::uniform_int_distribution<int> gp(2, 8);
    std::uniform_int_distribution<int> startY(0, HEIGHT - 1);
    std::bernoulli_distribution wake(0.65);

    auto resetCol = [&](Column& c) {
        c.speed = spd(rng);
        c.trail = trl(rng);
        c.gap = gp(rng);
        c.cool = 0;
        c.headY = startY(rng);
        c.active = true;
        };

    for (int x = 0; x < WIDTH; ++x) {
        cols[x].x = x;
        if (wake(rng)) resetCol(cols[x]);
        else {
            cols[x].active = false;
            cols[x].cool = gp(rng);
            cols[x].headY = startY(rng);
            cols[x].trail = trl(rng);
            cols[x].speed = spd(rng);
            cols[x].gap = gp(rng);
        }
    }

    // 셀 버퍼
    std::vector<std::string> chBuf(HEIGHT, std::string(WIDTH, ' '));
    std::vector<std::uint8_t> inten(HEIGHT * WIDTH, 0); // 0=없음,1=꼬리,2=몸통,3=헤드,4=ASCIIfix

    auto putCell = [&](int y, int x, char c, std::uint8_t level) {
        if (y < 0 || y >= HEIGHT || x < 0 || x >= WIDTH) return;
        chBuf[y][x] = c;
        size_t idx = (size_t)y * WIDTH + x;
        if (level >= inten[idx]) inten[idx] = level;
        };

    auto drawFrame = [&]() {
        std::cout << "\x1b[H";
        for (int r = 0; r < HEIGHT; ++r) {
            for (int c = 0; c < WIDTH; ++c) {
                char out = chBuf[r][c];
                std::uint8_t lv = inten[(size_t)r * WIDTH + c];
                if (lv == 0 || out == ' ') { std::cout << ' '; continue; }
                if (lv == 4) { std::cout << BOLD << GREEN << out << RESET; continue; }
                if (lv == 3) { std::cout << BOLD << GREEN << out << RESET; continue; }
                if (lv == 2) { std::cout << GREEN << out << RESET; continue; }
                if (lv == 1) { std::cout << DIM << GREEN << out << RESET; continue; }
            }
            if (r != HEIGHT - 1) std::cout << '\n';
        }
        };

    auto stepRain = [&](int ticks = 1) {
        while (ticks--) {
            for (int r = 0; r < HEIGHT; ++r) fill(chBuf[r].begin(), chBuf[r].end(), ' ');
            fill(inten.begin(), inten.end(), 0);

            for (auto& c : cols) {
                if (!c.active) {
                    if (--c.cool <= 0) { resetCol(c); }
                    continue;
                }
                c.headY = (c.headY + c.speed) % HEIGHT;

                for (int k = 0; k < c.trail; ++k) {
                    int y = c.headY - k; if (y < 0) y += HEIGHT;
                    char rc = randChar();
                    if (k == 0) putCell(y, c.x, rc, 3);
                    else if (k < 2) putCell(y, c.x, rc, 2);
                    else putCell(y, c.x, rc, 1);
                }

                if (c.headY % (HEIGHT + c.trail) == 0) {
                    c.active = false;
                    c.cool = c.gap;
                }
            }
        }
        };

    // ===== 4) 1단계: 레인만 흘리기 (풀화면) =====
    const int rainOnlyFrames = 40;
    for (int f = 0; f < rainOnlyFrames; ++f) {
        stepRain(1);
        drawFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    // ===== 5) 2단계: 타겟으로 수렴 (위에 덮어쓰며 고정) =====
    struct Pos { int r, c; };
    std::vector<Pos> targets;
    for (int r = 0; r < (int)lines.size(); ++r)
        for (int c = 0; c < (int)lines[r].size(); ++c)
            if (lines[r][c] != ' ') targets.push_back({ r, c });
    std::shuffle(targets.begin(), targets.end(), rng);

    const int revealFrames = 70;
    for (int f = 0; f < revealFrames; ++f) {
        stepRain(1);

        int revealCount = (int)((targets.size() * (f + 1)) / (double)revealFrames);

        for (int i = 0; i < revealCount; ++i) {
            int rr = offsetY + targets[i].r;
            int cc = offsetX + targets[i].c;
            putCell(rr, cc, lines[targets[i].r][targets[i].c], 4);
        }

        drawFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    // ===== 6) 최종 타겟 고정, 약간 유지 =====
    {
        stepRain(1);
        for (int r = 0; r < (int)lines.size(); ++r) {
            for (int c = 0; c < (int)lines[r].size(); ++c) {
                if (lines[r][c] == ' ') continue;
                putCell(offsetY + r, offsetX + c, lines[r][c], 4);
            }
        }
        drawFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // 커서 복구 & 클린업
    std::cout << "\x1b[?25h" << RESET;
    ConsoleUtils::clearScreen();
}




void Renderer::renderMainMenu_Static() {
    ConsoleUtils::clearScreen();

    const std::string intro = u8R"(

  PROJECT NAME : NINE_LIVES
  {
      <load = C:\source\repos\Nine_Lives>
      Please wait...................................................................................................100%
      Complete!
  }
)";

    const std::string asciiArt = u8R"(
                 
░▒▓███████▓▒░░▒▓█▓▒░▒▓███████▓▒░░▒▓████████▓▒░    ░▒▓█▓▒░      ░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓████████▓▒░░▒▓███████▓▒░ 
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░           ░▒▓█▓▒░      ░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░        
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░           ░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░        
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓██████▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒▒▓█▓▒░░▒▓██████▓▒░  ░▒▓██████▓▒░  
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░           ░▒▓█▓▒░      ░▒▓█▓▒░ ░▒▓█▓▓█▓▒░ ░▒▓█▓▒░             ░▒▓█▓▒░ 
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░           ░▒▓█▓▒░      ░▒▓█▓▒░ ░▒▓█▓▓█▓▒░ ░▒▓█▓▒░             ░▒▓█▓▒░ 
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓████████▓▒░    ░▒▓████████▓▒░▒▓█▓▒░  ░▒▓██▓▒░  ░▒▓████████▓▒░▒▓███████▓▒░  






)";


    const std::string outro = u8R"(
  .....................................................................................................................)";

    while (_kbhit()) _getch();
    bool isPrinting = true;

    std::cout << RED;

    // 1) intro 타이핑 출력 (기존처럼)
    for (char ch : intro) {
        std::cout << ch << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (_kbhit()) _getch();
    }
    //std::cout << RESET;

    // 2) asciiArt 부분 처리
    // - 아스키 아트 출력 시작 위치 고정 (예: 행 15, 열 10)
    // - 영역 크기: 아스키 아트 크기
    // - 매 프레임 열 하나씩 세로 방향으로 출력하면서 그 영역만 업데이트

    // 라인 분리 + 최대 너비 계산 + 길이 맞추기
    std::vector<std::string> lines;
    {
        std::stringstream ss(asciiArt);
        std::string line;
        size_t maxLen = 0;
        while (getline(ss, line)) {
            lines.push_back(line);
            maxLen = max(maxLen, line.size());
        }
        for (auto& line : lines) {
            if (line.size() < maxLen) line.append(maxLen - line.size(), ' ');
        }
    }

    size_t height = lines.size();
    size_t width = lines[0].size();

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // 아스키 아트 출력 시작 좌표 (원하는 위치로 조정)
    const SHORT startX = 7;
    const SHORT startY = 11;

    // 화면에 그려질 현재 상태 버퍼 (빈 공백으로 초기화)
    std::vector<std::string> screenBuffer(height, std::string(width, ' '));

    for (size_t col = 0; col < width; ++col) {
        // 이번 열 문자들 screenBuffer에 복사
        for (size_t row = 0; row < height; ++row) {
            screenBuffer[row][col] = lines[row][col];
        }

        // 화면에 현재 screenBuffer 출력 (영역만)
        for (size_t row = 0; row < height; ++row) {
            COORD cursorPos = { (SHORT)(startX), (SHORT)(startY + row) };
            SetConsoleCursorPosition(hOut, cursorPos);
            std::cout.write(screenBuffer[row].data(), screenBuffer[row].size());
        }
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        while (_kbhit()) _getch();

    }

    // 3) outro 타이핑 출력 (기존처럼)
    for (char ch : outro) {
        std::cout << ch << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (_kbhit()) _getch();

    }
    std::cout << RESET;
    std::cout << std::endl;
    cout << "\n";
}


void Renderer::renderMainMenu_Dynamic(int selection, bool canLoad) {
    std::vector<std::string> menuItems = { u8"새 게임 시작", u8"이전 게임 불러오기", u8"종료" };
    int start_line = 25; // 메뉴 항목을 출력할 시작 줄 번호 (조정 가능)

    for (int i = 0; i < menuItems.size(); ++i) {
        std::string menuItemText;
        std::string finalColor;

        if (i == 1 && !canLoad) { // '이전 게임 불러오기'가 불가능할 때
            finalColor = GRAY;
            menuItemText = u8"  " + menuItems[i];
        }
        else if (i == selection) { // 현재 선택된 항목
            finalColor = YELLOW;
            menuItemText = u8"➤ " + menuItems[i];
        }
        else { // 그 외 항목
            finalColor = WHITE;
            menuItemText = u8"  " + menuItems[i];
        }

        // 특정 라인에 중앙 정렬하여 출력
        std::cout << finalColor;
        printCentered(menuItemText, start_line + i);
        std::cout << RESET;
    }
}


// 최종 게임 오버 애니메이션
void Renderer::showGameOverAnimation() {
    ConsoleUtils::clearScreen();
    string msg = "\n\n   ALL CLONE BODIES DEPLETED. COMPLETE DEATH IMMINENT...\n";
    for (char c : msg) {
        cout << RED << c << RESET << flush;
        this_thread::sleep_for(chrono::milliseconds(30));
    }
    this_thread::sleep_for(chrono::milliseconds(1000));
    cout << RED << "\n   REVENGE HAS FAILED." << RESET << endl;
    this_thread::sleep_for(chrono::milliseconds(2000));
    cout << "\n\n   (PRESS ANY KEY TO EXIT)" << endl;
    _getch();
}