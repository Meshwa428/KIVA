#include "Animation.h"
#include <Arduino.h>
#include <math.h>

// --- VerticalListAnimation ---
void VerticalListAnimation::resize(size_t size) {
    itemOffsetY.assign(size, 0.0f);
    itemScale.assign(size, 0.0f);
    targetOffsetY.assign(size, 0.0f);
    targetScale.assign(size, 0.0f);
    introStartSourceOffsetY.assign(size, 0.0f);
    introStartSourceScale.assign(size, 0.0f);
}

void VerticalListAnimation::init() {
    std::fill(itemOffsetY.begin(), itemOffsetY.end(), 0.0f);
    std::fill(itemScale.begin(), itemScale.end(), 0.0f);
    std::fill(targetOffsetY.begin(), targetOffsetY.end(), 0.0f);
    std::fill(targetScale.begin(), targetScale.end(), 0.0f);
    std::fill(introStartSourceOffsetY.begin(), introStartSourceOffsetY.end(), 0.0f);
    std::fill(introStartSourceScale.begin(), introStartSourceScale.end(), 0.0f);
    isIntroPhase = false; introStartTime = 0;
}

void VerticalListAnimation::setTargets(int selIdx, int total, float itemSpacing) {
    if (targetOffsetY.size() != total) resize(total);
    for (int i = 0; i < total; i++) {
        if (i < total) {
            int rP = i - selIdx;
            targetOffsetY[i] = rP * itemSpacing;
            if (i == selIdx) targetScale[i] = 1.3f;
            else if (abs(rP) == 1) targetScale[i] = 1.f;
            else targetScale[i] = 0.8f;
        } else {
            targetOffsetY[i] = (i - selIdx) * itemSpacing;
            targetScale[i] = 0.f;
        }
    }
}

void VerticalListAnimation::startIntro(int selIdx, int total, float itemSpacing) {
    isIntroPhase = true;
    introStartTime = millis();
    const float initial_scale = 0.0f;
    const float initial_y_offset_factor_from_final = 0.25f;

    for (int i = 0; i < total; i++) {
        if (i < total) {
            int rP = i - selIdx;
            float finalTargetOffsetY = rP * itemSpacing;
            float finalTargetScale = (i == selIdx) ? 1.3f : (abs(rP) == 1) ? 1.f : 0.8f;
            introStartSourceOffsetY[i] = finalTargetOffsetY * (1.0f - initial_y_offset_factor_from_final);
            introStartSourceScale[i] = initial_scale;
            itemOffsetY[i] = introStartSourceOffsetY[i];
            itemScale[i] = introStartSourceScale[i];
            targetOffsetY[i] = finalTargetOffsetY;
            targetScale[i] = finalTargetScale;
        } else {
            introStartSourceOffsetY[i] = (i - selIdx) * itemSpacing;
            introStartSourceScale[i] = 0.0f;
            itemOffsetY[i] = introStartSourceOffsetY[i];
            itemScale[i] = introStartSourceScale[i];
            targetOffsetY[i] = (i - selIdx) * itemSpacing;
            targetScale[i] = 0.0f;
        }
    }
}

bool VerticalListAnimation::update() {
    bool animActive = false;
    unsigned long currentTime = millis();
    if (isIntroPhase) {
        float progress = (introStartTime > 0 && currentTime > introStartTime && introDuration > 0) ? (float)(currentTime - introStartTime) / introDuration : 0.0f;
        if (progress >= 1.0f) {
            isIntroPhase = false;
            for (size_t i = 0; i < itemOffsetY.size(); i++) { itemOffsetY[i] = targetOffsetY[i]; itemScale[i] = targetScale[i]; }
            return false;
        }
        float easedProgress = 1.0f - pow(1.0f - progress, 3);
        for (size_t i = 0; i < itemOffsetY.size(); i++) {
            itemOffsetY[i] = introStartSourceOffsetY[i] + (targetOffsetY[i] - introStartSourceOffsetY[i]) * easedProgress;
            itemScale[i] = introStartSourceScale[i] + (targetScale[i] - introStartSourceScale[i]) * easedProgress;
        }
        return true;
    } else {
        for (size_t i = 0; i < itemOffsetY.size(); i++) {
            float oD = targetOffsetY[i] - itemOffsetY[i];
            if (abs(oD) > 0.01f) { itemOffsetY[i] += oD * animSpd * frmTime; animActive = true; } else { itemOffsetY[i] = targetOffsetY[i]; }
            float sD = targetScale[i] - itemScale[i];
            if (abs(sD) > 0.001f) { itemScale[i] += sD * animSpd * frmTime; animActive = true; } else { itemScale[i] = targetScale[i]; }
        }
    }
    return animActive;
}

// --- CarouselAnimation ---
void CarouselAnimation::resize(size_t size) {
    itemOffsetX.assign(size, 0.0f);
    itemScale.assign(size, 0.0f);
    targetOffsetX.assign(size, 0.0f);
    targetScale.assign(size, 0.0f);
}

void CarouselAnimation::init() {
    std::fill(itemOffsetX.begin(), itemOffsetX.end(), 0.0f);
    std::fill(itemScale.begin(), itemScale.end(), 0.0f);
    std::fill(targetOffsetX.begin(), targetOffsetX.end(), 0.0f);
    std::fill(targetScale.begin(), targetScale.end(), 0.0f);
}

void CarouselAnimation::setTargets(int selIdx, int total) {
    for (int i = 0; i < total; i++) {
        if (i < total) {
            int rP = i - selIdx;
            targetOffsetX[i] = rP * (cardBaseW * 0.8f + cardGap);
            if (rP == 0) targetScale[i] = 1.f;
            else if (abs(rP) == 1) targetScale[i] = 0.75f;
            else if (abs(rP) == 2) targetScale[i] = 0.5f;
            else targetScale[i] = 0.f;
        } else {
            targetScale[i] = 0.f;
            targetOffsetX[i] = (i - selIdx) * (cardBaseW * 0.8f + cardGap);
        }
    }
}

bool CarouselAnimation::update() {
    bool anim = false;
    for (size_t i = 0; i < itemOffsetX.size(); i++) {
        float oD = targetOffsetX[i] - itemOffsetX[i];
        if (abs(oD) > 0.1f) { itemOffsetX[i] += oD * animSpd * frmTime; anim = true; } else { itemOffsetX[i] = targetOffsetX[i]; }
        float sD = targetScale[i] - itemScale[i];
        if (abs(sD) > 0.01f) { itemScale[i] += sD * animSpd * frmTime; anim = true; } else { itemScale[i] = targetScale[i]; }
    }
    return anim;
}