#ifndef ANIMATION_H
#define ANIMATION_H

#include "Config.h"

struct VerticalListAnimation {
    float itemOffsetY[MAX_ANIM_ITEMS], itemScale[MAX_ANIM_ITEMS];
    float targetOffsetY[MAX_ANIM_ITEMS], targetScale[MAX_ANIM_ITEMS];
    float introStartSourceOffsetY[MAX_ANIM_ITEMS];
    float introStartSourceScale[MAX_ANIM_ITEMS];
    bool isIntroPhase;
    unsigned long introStartTime;
    static constexpr float animSpd = 12.f;
    static constexpr float frmTime = 0.016f;
    static constexpr float itmSpc = 18.f;
    static constexpr float introDuration = 500.0f;

    void init();
    void startIntro(int selIdx, int total);
    void setTargets(int selIdx, int total);
    bool update();
};

struct CarouselAnimation {
    float itemOffsetX[MAX_ANIM_ITEMS], itemScale[MAX_ANIM_ITEMS];
    float targetOffsetX[MAX_ANIM_ITEMS], targetScale[MAX_ANIM_ITEMS];
    const float animSpd = 12.f, frmTime = 0.016f;
    const float cardBaseW = 58.f, cardBaseH = 42.f, cardGap = 6.f;

    void init();
    void setTargets(int selIdx, int total);
    bool update();
};

#endif // ANIMATION_H