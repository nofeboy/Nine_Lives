#pragma once
#include "EventManager.h"
#include "Player.h"
#include "Input.h"
#include <string>

class Renderer {
public:
    // ✅ 전체 이벤트 화면 렌더링 (HUD + 카드 + 선택지 + 아이템)
    static void renderEventFull(const Event& ev, const Player& player, bool animateCard, int turnCount, int scenarioCount);

    // ✅ 선택지 강조만 갱신 (예비 선택 시)
    static void updateChoicesOnly(const Event& ev, const Player& player, Direction previewDir);

    // ✅ 카드 내부 텍스트 지우기 (테두리 유지)
    static void eraseCardText(const std::string& text);

    // ✅ HUD + 아이템 출력
    static void drawHUD(const Player& player, int turnCount, int scenarioCount);
    static void drawItems(const Player& player);

    //선택지 출력
    static void renderChoices(const Event& ev, const Player& player, Direction previewDir);

    // ✅ 상태 메시지
    static void showLockedWarning();
    static void showGameOver();
    static void showCompleteGameOver();
    static void showCloneRevival();
    //static void showItemDrop(const std::string& itemName);
    static void showReviveAnimation();
};

// ✅ 내부 카드 출력용 (cpp 전용에서 정의)
// drawCardInstant() → 카드 즉시 출력
// drawCardAnimated() → 카드 애니메이션 출력
