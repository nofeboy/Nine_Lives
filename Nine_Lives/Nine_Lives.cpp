//#include <iostream>
//#include <conio.h>
//#include <windows.h>
//#include <chrono>
//#include <thread>
//#include <vector>
//#include <map>
//#include <unordered_map>
//#include <fstream>
//#include <nlohmann/json.hpp>
//
//using json = nlohmann::json;
//using namespace std;
//
//struct Choice {
//    string text;
//    int reqStrength;
//    int reqHacking;
//    int hpDelta;
//    int sanityDelta;
//    int moneyDelta;
//    string nextEventId;
//};
//
//struct Event {
//    string id;
//    string description;
//    vector<Choice> choices;
//};
//
//struct Player {
//    int strength = 0;
//    int hacking = 0;
//    int hp = 7;
//    int sanity = 7;
//    int money = 3;
//    map<string, int> items;
//};
//
//unordered_map<string, Event> events;
//
//// 콘솔 클리어
//void clearScreen() {
//    system("cls");
//}
//
//// 타이핑 효과
//void typeText(const string& text, int delay = 30) {
//    for (char c : text) {
//        cout << c << flush;
//        this_thread::sleep_for(chrono::milliseconds(delay));
//    }
//    cout << "\n";
//}
//
//// 텍스트 지우기 (애니메이션)
//void eraseText(string& text, int delay = 10) {
//    for (int i = text.size(); i >= 0; --i) {
//        cout << "\r" << string(i, ' ') << "\r" << flush;
//        this_thread::sleep_for(chrono::milliseconds(delay));
//    }
//}
//
//// JSON 로드
//void loadEvents(const string& filename) {
//    ifstream file(filename);
//    json data;
//    file >> data;
//
//    for (auto& e : data["events"]) {
//        Event ev;
//        ev.id = e["id"];
//        ev.description = e["description"];
//        for (auto& c : e["choices"]) {
//            Choice ch;
//            ch.text = c["text"];
//            ch.reqStrength = c["reqStrength"];
//            ch.reqHacking = c["reqHacking"];
//            ch.hpDelta = c["hpDelta"];
//            ch.sanityDelta = c["sanityDelta"];
//            ch.moneyDelta = c["moneyDelta"];
//            ch.nextEventId = c["nextEventId"];
//            ev.choices.push_back(ch);
//        }
//        events[ev.id] = ev;
//    }
//}
//
//// 방향키 코드
//enum Direction { NONE, UP, DOWN, LEFT, RIGHT };
//
//Direction getDirection() {
//    int ch = _getch();
//    if (ch == 224) { // 방향키 prefix
//        int dir = _getch();
//        switch (dir) {
//        case 72: return UP;
//        case 80: return DOWN;
//        case 75: return LEFT;
//        case 77: return RIGHT;
//        }
//    }
//    return NONE;
//}
//
//// 선택 UI 렌더링
//void renderEvent(const Event& ev, int offsetX, int offsetY) {
//    clearScreen();
//    cout << "\n";
//    // 중앙 카드
//    cout << string(40 + offsetX, ' ') << "[EVENT]\n";
//    cout << string(40 + offsetX, ' ') << ev.description << "\n\n";
//
//    // 선택지 위치
//    for (int i = 0; i < ev.choices.size(); i++) {
//        switch (i) {
//        case 0: cout << string(40, ' ') << "[→] " << ev.choices[i].text << "\n"; break;
//        case 1: cout << string(20, ' ') << "[←] " << ev.choices[i].text << "\n"; break;
//        case 2: cout << string(40, ' ') << "[↑] " << ev.choices[i].text << "\n"; break;
//        case 3: cout << string(40, ' ') << "[↓] " << ev.choices[i].text << "\n"; break;
//        }
//    }
//}
//
//// 카드 움직임 애니메이션
//pair<int, int> applyPreviewOffset(Direction dir) {
//    switch (dir) {
//    case UP: return { 0, -1 };
//    case DOWN: return { 0, 1 };
//    case LEFT: return { -10, 0 };
//    case RIGHT: return { 10, 0 };
//    default: return { 0, 0 };
//    }
//}
//
//int main() {
//    // 콘솔 출력 UTF-8 설정
//    SetConsoleOutputCP(CP_UTF8);
//    SetConsoleCP(CP_UTF8);
//
//    // 버퍼 설정 (한글 깨짐 방지)
//    std::ios::sync_with_stdio(false);
//    std::cin.tie(nullptr);
//
//    loadEvents("events.json");
//
//    Player player;
//    Event currentEvent = events["intro_1"];
//
//    Direction lastDir = NONE;
//    bool previewMode = false;
//
//    while (true) {
//        pair<int, int> offset = previewMode ? applyPreviewOffset(lastDir) : make_pair(0, 0);
//        renderEvent(currentEvent, offset.first, offset.second);
//
//        Direction dir = getDirection();
//        if (dir == NONE) continue;
//
//        if (!previewMode) {
//            lastDir = dir;
//            previewMode = true;
//        }
//        else if (previewMode && dir == lastDir) {
//            // 선택 확정
//            int choiceIndex = (dir == RIGHT ? 0 : dir == LEFT ? 1 : dir == UP ? 2 : 3);
//            if (choiceIndex < currentEvent.choices.size()) {
//                Choice& ch = currentEvent.choices[choiceIndex];
//
//                // 텍스트 삭제 애니메이션
//                string text = currentEvent.description;
//                eraseText(text);
//
//                // 다음 이벤트 출력
//                currentEvent = events[ch.nextEventId];
//                previewMode = false;
//            }
//        }
//        else {
//            lastDir = dir;
//        }
//    }
//
//    return 0;
//}
