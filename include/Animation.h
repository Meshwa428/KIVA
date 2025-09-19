#ifndef ANIMATION_H
#define ANIMATION_H

#include <vector>

struct VerticalListAnimation {
    std::vector<float> itemOffsetY, itemScale;
    std::vector<float> targetOffsetY, targetScale;
    std::vector<float> introStartSourceOffsetY;
    std::vector<float> introStartSourceScale;
    bool isIntroPhase;
    unsigned long introStartTime;
    static constexpr float animSpd = 12.f;
    static constexpr float frmTime = 0.016f;
    static constexpr float itmSpc = 18.f;
    static constexpr float introDuration = 500.0f;
    
    void resize(size_t size);
    void init();
    void startIntro(int selIdx, int total, float itemSpacing = itmSpc);
    void setTargets(int selIdx, int total, float itemSpacing = itmSpc);
    bool update();
};

struct CarouselAnimation {
    std::vector<float> itemOffsetX, itemScale;
    std::vector<float> targetOffsetX, targetScale;
    const float animSpd = 12.f, frmTime = 0.016f;
    const float cardBaseW = 58.f, cardBaseH = 42.f, cardGap = 6.f;

    void resize(size_t size);
    void init();
    void setTargets(int selIdx, int total);
    bool update();
};

struct GridAnimation {
    std::vector<float> itemScale;
    std::vector<unsigned long> itemAnimStartTime;
    float targetScrollOffset_Y;
    float currentScrollOffset_Y;
    bool isAnimatingIn;

    static constexpr float animSpd = 20.f;
    static constexpr float frmTime = 0.016f;
    static constexpr float staggerDelay = 40.0f;

    void resize(size_t size);
    void init();
    void startIntro(int numItems, int columns);
    void setScrollTarget(float target);
    bool update();
};

#endif // ANIMATION_H