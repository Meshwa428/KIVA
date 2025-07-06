#include "Animation.h"
#include <Arduino.h>
#include <math.h>

// --- VerticalListAnimation ---
void VerticalListAnimation::init() {
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        itemOffsetY[i] = 0; itemScale[i] = 0.0f;
        targetOffsetY[i] = 0; targetScale[i] = 0.0f;
        introStartSourceOffsetY[i] = 0.0f; introStartSourceScale[i] = 0.0f;
    }
    isIntroPhase = false; introStartTime = 0;
}

void VerticalListAnimation::setTargets(int selIdx, int total) {
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        if (i < total) {
            int rP = i - selIdx;
            targetOffsetY[i] = rP * itmSpc;
            if (i == selIdx) targetScale[i] = 1.3f;
            else if (abs(rP) == 1) targetScale[i] = 1.f;
            else targetScale[i] = 0.8f;
        } else {
            targetOffsetY[i] = (i - selIdx) * itmSpc;
            targetScale[i] = 0.f;
        }
    }
}

void VerticalListAnimation::startIntro(int selIdx, int total) {
    isIntroPhase = true;
    introStartTime = millis();
    const float initial_scale = 0.0f;
    const float initial_y_offset_factor_from_final = 0.25f;

    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        if (i < total) {
            int rP = i - selIdx;
            float finalTargetOffsetY = rP * itmSpc;
            float finalTargetScale = (i == selIdx) ? 1.3f : (abs(rP) == 1) ? 1.f : 0.8f;
            introStartSourceOffsetY[i] = finalTargetOffsetY * (1.0f - initial_y_offset_factor_from_final);
            introStartSourceScale[i] = initial_scale;
            itemOffsetY[i] = introStartSourceOffsetY[i];
            itemScale[i] = introStartSourceScale[i];
            targetOffsetY[i] = finalTargetOffsetY;
            targetScale[i] = finalTargetScale;
        } else {
            introStartSourceOffsetY[i] = (i - selIdx) * itmSpc;
            introStartSourceScale[i] = 0.0f;
            itemOffsetY[i] = introStartSourceOffsetY[i];
            itemScale[i] = introStartSourceScale[i];
            targetOffsetY[i] = (i - selIdx) * itmSpc;
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
            for (int i = 0; i < MAX_ANIM_ITEMS; i++) { itemOffsetY[i] = targetOffsetY[i]; itemScale[i] = targetScale[i]; }
            return false;
        }
        float easedProgress = 1.0f - pow(1.0f - progress, 3);
        for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
            itemOffsetY[i] = introStartSourceOffsetY[i] + (targetOffsetY[i] - introStartSourceOffsetY[i]) * easedProgress;
            itemScale[i] = introStartSourceScale[i] + (targetScale[i] - introStartSourceScale[i]) * easedProgress;
        }
        return true;
    } else {
        for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
            float oD = targetOffsetY[i] - itemOffsetY[i];
            if (abs(oD) > 0.01f) { itemOffsetY[i] += oD * animSpd * frmTime; animActive = true; } else { itemOffsetY[i] = targetOffsetY[i]; }
            float sD = targetScale[i] - itemScale[i];
            if (abs(sD) > 0.001f) { itemScale[i] += sD * animSpd * frmTime; animActive = true; } else { itemScale[i] = targetScale[i]; }
        }
    }
    return animActive;
}

// --- CarouselAnimation ---
void CarouselAnimation::init() {
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        itemOffsetX[i] = 0; itemScale[i] = 0.f;
        targetOffsetX[i] = 0; targetScale[i] = 0.f;
    }
}

void CarouselAnimation::setTargets(int selIdx, int total) {
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
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
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        float oD = targetOffsetX[i] - itemOffsetX[i];
        if (abs(oD) > 0.1f) { itemOffsetX[i] += oD * animSpd * frmTime; anim = true; } else { itemOffsetX[i] = targetOffsetX[i]; }
        float sD = targetScale[i] - itemScale[i];
        if (abs(sD) > 0.01f) { itemScale[i] += sD * animSpd * frmTime; anim = true; } else { itemScale[i] = targetScale[i]; }
    }
    return anim;
}