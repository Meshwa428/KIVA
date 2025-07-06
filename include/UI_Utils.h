#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <U8g2lib.h>
#include "Icons.h" // For IconType enum

// --- NEW: Enum for icon size ---
enum class IconRenderSize
{
    LARGE,
    SMALL
};

// --- Drawing Utilities ---
// MODIFIED: Signature now includes IconRenderSize
void drawCustomIcon(U8G2 &display, int x, int y, IconType iconType, IconRenderSize size = IconRenderSize::LARGE);
void drawRndBox(U8G2 &display, int x, int y, int w, int h, int r, bool fill);

// --- Text & Marquee Utilities ---
char *truncateText(const char *text, int maxWidth, U8G2 &display);
void updateMarquee(bool &marqueeActive, bool &marqueePaused, bool &marqueeScrollLeft,
                   unsigned long &marqueePauseStartTime, unsigned long &lastMarqueeTime,
                   float &marqueeOffset, char *marqueeText, int &marqueeTextLenPx,
                   const char *textToDisplay, int availableWidth, U8G2 &display);
void drawBatIcon(U8G2 &display, int x, int y, uint8_t percentage);

#endif // UI_UTILS_H