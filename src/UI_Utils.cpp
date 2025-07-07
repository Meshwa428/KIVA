#include "UI_Utils.h"
#include <Arduino.h>

// Buffer for truncateText. It's static, so it's private to this file.
static char SBUF[32];

void drawCustomIcon(U8G2 &display, int x, int y, IconType iconType, IconRenderSize size)
{
    const unsigned char *xbm_data = nullptr;
    int w = 0, h = 0;
    bool icon_resolved = false;

    if (size == IconRenderSize::SMALL)
    {
        switch (iconType)
        {
        case IconType::UI_CHARGING_BOLT:
            xbm_data = icon_ui_charging_bolt_small_bits;
            w = IconSize::SMALL_WIDTH;
            h = IconSize::SMALL_HEIGHT;
            icon_resolved = true;
            break;
        case IconType::UI_REFRESH:
            xbm_data = icon_ui_refresh_small_bits;
            w = IconSize::SMALL_WIDTH;
            h = IconSize::SMALL_HEIGHT;
            icon_resolved = true;
            break;
        case IconType::NAV_BACK:
            xbm_data = icon_nav_back_small_bits;
            w = IconSize::SMALL_WIDTH;
            h = IconSize::SMALL_HEIGHT;
            icon_resolved = true;
            break;
        case IconType::NET_WIFI_LOCK:
            xbm_data = icon_lock_small_bits;
            w = IconSize::SMALL_WIDTH;
            h = IconSize::SMALL_HEIGHT;
            icon_resolved = true;
        default:
            break; // Fall through to large icon
        }
    }

    if (!icon_resolved)
    {
        w = IconSize::LARGE_WIDTH;
        h = IconSize::LARGE_HEIGHT;
        switch (iconType)
        {
        case IconType::BOOT_LOGO:
            xbm_data = icon_boot_logo_bits;
            w = IconSize::BOOT_LOGO_WIDTH;
            h = IconSize::BOOT_LOGO_HEIGHT;
            break;
        case IconType::TOOLS:
            xbm_data = icon_tools_large_bits;
            break;
        case IconType::GAMES:
            xbm_data = icon_games_large_bits;
            break;
        case IconType::SETTINGS:
            xbm_data = icon_settings_large_bits;
            break;
        case IconType::INFO:
            xbm_data = icon_info_large_bits;
            break;
        case IconType::GAME_SNAKE:
            xbm_data = icon_game_snake_large_bits;
            break;
        case IconType::GAME_TETRIS:
            xbm_data = icon_game_tetris_large_bits;
            break;
        case IconType::GAME_PONG:
            xbm_data = icon_game_pong_large_bits;
            break;
        case IconType::GAME_MAZE:
            xbm_data = icon_game_maze_large_bits;
            break;
        case IconType::NAV_BACK:
            xbm_data = icon_nav_back_large_bits;
            break;
        case IconType::NET_WIFI:
            xbm_data = icon_net_wifi_large_bits;
            break;
        case IconType::NET_BLUETOOTH:
            xbm_data = icon_net_bluetooth_large_bits;
            break;
        case IconType::TOOL_JAMMING:
            xbm_data = icon_tool_jamming_large_bits;
            break;
        case IconType::TOOL_INJECTION:
            xbm_data = icon_tool_injection_large_bits;
            break;
        case IconType::SETTING_DISPLAY:
            xbm_data = icon_setting_display_large_bits;
            break;
        case IconType::SETTING_SOUND:
            xbm_data = icon_setting_sound_large_bits;
            break;
        case IconType::SETTING_SYSTEM:
            xbm_data = icon_setting_system_large_bits;
            break;
        case IconType::UI_REFRESH:
            xbm_data = icon_ui_refresh_large_bits;
            break;
        case IconType::UI_CHARGING_BOLT:
            xbm_data = icon_ui_charging_bolt_large_bits;
            break;
        case IconType::UI_VIBRATION:
            xbm_data = icon_ui_vibration_large_bits;
            break;
        case IconType::UI_LASER:
            xbm_data = icon_ui_laser_large_bits;
            break;
        case IconType::UI_FLASHLIGHT:
            xbm_data = icon_ui_flashlight_large_bits;
            break;
        case IconType::UTILITIES_CATEGORY:
            xbm_data = icon_utilities_category_large_bits;
            break;
        default:
            xbm_data = nullptr;
            break;
        }
    }

    if (xbm_data && w > 0 && h > 0)
    {
        display.drawXBM(x, y, w, h, xbm_data);
    }
    else
    {
        int placeholder_w = (size == IconRenderSize::SMALL) ? IconSize::SMALL_WIDTH : IconSize::LARGE_WIDTH;
        int placeholder_h = (size == IconRenderSize::SMALL) ? IconSize::SMALL_HEIGHT : IconSize::LARGE_HEIGHT;
        display.drawFrame(x, y, placeholder_w, placeholder_h);
        display.drawLine(x, y, x + placeholder_w - 1, y + placeholder_h - 1);
    }
}

void drawRndBox(U8G2 &display, int x, int y, int w, int h, int r, bool fill)
{
    if (w <= 0 || h <= 0)
        return;
    if (w <= 2 * r || h <= 2 * r)
    {
        if (fill)
            display.drawBox(x, y, w, h);
        else
            display.drawFrame(x, y, w, h);
        return;
    }
    if (fill)
        display.drawRBox(x, y, w, h, r);
    else
        display.drawRFrame(x, y, w, h, r);
}

// --- CORRECTED FUNCTION ---
char *truncateText(const char *text, int maxWidth, U8G2 &display)
{
    // 1. Copy original text to the static buffer, respecting its size.
    strncpy(SBUF, text, sizeof(SBUF) - 1);
    SBUF[sizeof(SBUF) - 1] = '\0';

    // 2. Check if the copied text already fits within the pixel width.
    if (display.getStrWidth(SBUF) <= maxWidth)
    {
        return SBUF;
    }

    // 3. If not, start removing characters from the end until it fits with an ellipsis.
    int ellipsisWidth = display.getStrWidth("...");
    while (strlen(SBUF) > 0)
    {
        // Remove the last character.
        SBUF[strlen(SBUF) - 1] = '\0';

        // Check if the now-shortened string plus the ellipsis will fit.
        if (display.getStrWidth(SBUF) + ellipsisWidth <= maxWidth)
        {
            strcat(SBUF, "..."); // Append the ellipsis.
            return SBUF;
        }
    }

    // 4. If we reach here, it means even the ellipsis itself might not fit.
    if (ellipsisWidth <= maxWidth)
    {
        strcpy(SBUF, "...");
        return SBUF;
    }

    // 5. If nothing fits, return an empty string.
    SBUF[0] = '\0';
    return SBUF;
}

void updateMarquee(bool &marqueeActive, bool &marqueePaused, bool &marqueeScrollLeft,
                   unsigned long &marqueePauseStartTime, unsigned long &lastMarqueeTime,
                   float &marqueeOffset, char *marqueeText, int &marqueeTextLenPx,
                   const char *textToDisplay, int availableWidth, U8G2 &display)
{

    bool shouldBeActive = display.getStrWidth(textToDisplay) > availableWidth;

    if (!marqueeActive && shouldBeActive)
    {
        // Marquee needs to start
        strncpy(marqueeText, textToDisplay, 39);
        marqueeText[39] = '\0';
        marqueeTextLenPx = display.getStrWidth(marqueeText);
        marqueeOffset = 0;
        marqueeActive = true;
        marqueePaused = true;
        marqueeScrollLeft = true; // Always start by scrolling left
        marqueePauseStartTime = millis();
        lastMarqueeTime = millis();
    }
    else if (marqueeActive && !shouldBeActive)
    {
        // Marquee should stop
        marqueeActive = false;
    }
    else if (marqueeActive && shouldBeActive)
    {
        // If the text changes while marquee is active
        if (strcmp(marqueeText, textToDisplay) != 0)
        {
            strncpy(marqueeText, textToDisplay, 39);
            marqueeText[39] = '\0';
            marqueeTextLenPx = display.getStrWidth(marqueeText);
            marqueeOffset = 0;
            marqueePaused = true;
            marqueeScrollLeft = true;
            marqueePauseStartTime = millis();
        }
    }

    if (!marqueeActive)
        return;

    unsigned long curTime = millis();
    if (marqueePaused)
    {
        if (curTime - marqueePauseStartTime > 1200)
        { // Pause for 1.2 seconds
            marqueePaused = false;
            lastMarqueeTime = curTime;
        }
        return;
    }

    if (curTime - lastMarqueeTime > 70)
    { // Update every 70ms
        if (marqueeScrollLeft)
        {
            marqueeOffset--; // Scroll left
        }
        else
        {
            marqueeOffset++; // Scroll right
        }
        lastMarqueeTime = curTime;

        // Check for end conditions
        if (marqueeScrollLeft && marqueeOffset <= -(marqueeTextLenPx - availableWidth))
        {
            marqueeOffset = -(marqueeTextLenPx - availableWidth); // Clamp position
            marqueePaused = true;
            marqueePauseStartTime = curTime;
            marqueeScrollLeft = false; // Reverse direction
        }
        else if (!marqueeScrollLeft && marqueeOffset >= 0)
        {
            marqueeOffset = 0; // Clamp position
            marqueePaused = true;
            marqueePauseStartTime = curTime;
            marqueeScrollLeft = true; // Reverse direction
        }
    }
}

void drawBatIcon(U8G2 &display, int x, int y, uint8_t percentage)
{
    display.drawFrame(x, y, 9, 5);
    display.drawBox(x + 9, y + 1, 1, 3);
    int fill_width = (percentage * 7) / 100;
    if (fill_width > 0)
    {
        display.drawBox(x + 1, y + 1, fill_width, 3);
    }
}