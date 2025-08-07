#include "UI_Utils.h"
#include <Arduino.h>
#include <vector>

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
        case IconType::ERROR:
            xbm_data = icon_error_large_bits;
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
        case IconType::SD_CARD:
            xbm_data = icon_sd_card_large_bits;
            break;
        case IconType::BEACON:
            xbm_data = icon_beacon_large_bits;
            break;
        case IconType::SKULL:
            xbm_data = icon_skull_large_bits;
            break;
        case IconType::BASIC_OTA:
            xbm_data = icon_basic_ota_large_bits;
            break;
        case IconType::FIRMWARE_UPDATE:
            xbm_data = icon_firmware_update_large_bits;
            break;
        case IconType::DISCONNECT:
            xbm_data = icon_disconnect_large_bits;
            break;
        case IconType::USB:
            xbm_data = icon_usb_large_bits;
            break;
        case IconType::MUSIC_PLAYER:
            xbm_data = icon_music_player_large_bits;
            break;
        case IconType::PLAYLIST:
            xbm_data = icon_playlist_large_bits;
            break;
        case IconType::MUSIC_NOTE:
            xbm_data = icon_music_note_large_bits;
            break;
        case IconType::PLAY:
            xbm_data = icon_play_large_bits;
            break;
        case IconType::PAUSE:
            xbm_data = icon_pause_large_bits;
            break;
        case IconType::NEXT_TRACK:
            xbm_data = icon_next_track_large_bits;
            break;
        case IconType::PREV_TRACK:
            xbm_data = icon_prev_track_large_bits;
            break;
        case IconType::SHUFFLE:
            xbm_data = icon_shuffle_large_bits;
            break;
        case IconType::REPEAT:
            xbm_data = icon_repeat_large_bits;
            break;
        case IconType::REPEAT_ONE:
            xbm_data = icon_repeat_one_large_bits;
            break;
        case IconType::NONE:
            xbm_data = icon_none_large_bits;
            break;
        default:
            xbm_data = icon_none_large_bits;
            break;
        }
    }

    if (xbm_data && w > 0 && h > 0)
    {
        display.drawXBM(x, y, w, h, xbm_data);
    }
    // else
    // {
    //     int placeholder_w = (size == IconRenderSize::SMALL) ? IconSize::SMALL_WIDTH : IconSize::LARGE_WIDTH;
    //     int placeholder_h = (size == IconRenderSize::SMALL) ? IconSize::SMALL_HEIGHT : IconSize::LARGE_HEIGHT;
    //     display.drawFrame(x, y, placeholder_w, placeholder_h);
    //     display.drawLine(x, y, x + placeholder_w - 1, y + placeholder_h - 1);
    // }
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

char *truncateText(const char *text, int maxWidth, U8G2 &display)
{
    strncpy(SBUF, text, sizeof(SBUF) - 1);
    SBUF[sizeof(SBUF) - 1] = '\0';

    if (display.getStrWidth(SBUF) <= maxWidth)
    {
        return SBUF;
    }

    int ellipsisWidth = display.getStrWidth("...");
    while (strlen(SBUF) > 0)
    {
        SBUF[strlen(SBUF) - 1] = '\0';
        if (display.getStrWidth(SBUF) + ellipsisWidth <= maxWidth)
        {
            strcat(SBUF, "...");
            return SBUF;
        }
    }

    if (ellipsisWidth <= maxWidth)
    {
        strcpy(SBUF, "...");
        return SBUF;
    }

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
        strncpy(marqueeText, textToDisplay, 39);
        marqueeText[39] = '\0';
        marqueeTextLenPx = display.getStrWidth(marqueeText);
        marqueeOffset = 0;
        marqueeActive = true;
        marqueePaused = true;
        marqueeScrollLeft = true;
        marqueePauseStartTime = millis();
        lastMarqueeTime = millis();
    }
    else if (marqueeActive && !shouldBeActive)
    {
        marqueeActive = false;
    }
    else if (marqueeActive && shouldBeActive)
    {
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
        {
            marqueePaused = false;
            lastMarqueeTime = curTime;
        }
        return;
    }

    if (curTime - lastMarqueeTime > 70)
    {
        if (marqueeScrollLeft)
        {
            marqueeOffset--;
        }
        else
        {
            marqueeOffset++;
        }
        lastMarqueeTime = curTime;

        if (marqueeScrollLeft && marqueeOffset <= -(marqueeTextLenPx - availableWidth))
        {
            marqueeOffset = -(marqueeTextLenPx - availableWidth);
            marqueePaused = true;
            marqueePauseStartTime = curTime;
            marqueeScrollLeft = false;
        }
        else if (!marqueeScrollLeft && marqueeOffset >= 0)
        {
            marqueeOffset = 0;
            marqueePaused = true;
            marqueePauseStartTime = curTime;
            marqueeScrollLeft = true;
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

// --- NEW IMPLEMENTATION ---
void drawWrappedText(U8G2 &display, const char* text, int x, int y, int w, int h, const std::vector<const uint8_t*>& fonts) {
    if (!text || text[0] == '\0' || fonts.empty()) {
        return;
    }

    const uint8_t* best_font = fonts.back(); // Default to smallest font

    // 1. Find the best font size
    for (const auto& font : fonts) {
        display.setFont(font);
        if (display.getStrWidth(text) <= w) {
            best_font = font;
            break; // Found a font that fits on one line
        }
    }
    display.setFont(best_font);
    int line_height = display.getAscent() - display.getDescent() + 2;
    int max_lines = h / line_height;

    // 2. Word wrap the text
    std::vector<std::string> lines;
    std::string current_line;
    char text_copy[strlen(text) + 1];
    strcpy(text_copy, text);

    char* word = strtok(text_copy, " ");
    while (word != nullptr) {
        std::string word_str(word);
        if (current_line.empty()) {
            current_line = word_str;
        } else {
            std::string test_line = current_line + " " + word_str;
            if (display.getStrWidth(test_line.c_str()) <= w) {
                current_line = test_line;
            } else {
                lines.push_back(current_line);
                current_line = word_str;
            }
        }
        word = strtok(nullptr, " ");
    }
    if (!current_line.empty()) {
        lines.push_back(current_line);
    }

    if (lines.empty()) return;

    // 3. Draw the wrapped text, vertically centered
    int total_text_height = lines.size() * line_height;
    int start_y = y + (h - total_text_height) / 2 + display.getAscent();

    for (size_t i = 0; i < lines.size() && i < max_lines; ++i) {
        int line_width = display.getStrWidth(lines[i].c_str());
        int line_x = x + (w - line_width) / 2;
        display.drawStr(line_x, start_y + (i * line_height), lines[i].c_str());
    }
}