// KivaMain/icons.h
#ifndef ICONS_H
#define ICONS_H

#include <stdint.h> // For uint8_t

// Define Icon Dimensions
const int LARGE_ICON_WIDTH = 15;
const int LARGE_ICON_HEIGHT = 15;
const int SMALL_ICON_WIDTH = 7;  // Or your preferred small dimension
const int SMALL_ICON_HEIGHT = 7; // Or your preferred small dimension

// Enum for desired rendering size
enum IconRenderSize {
    // RENDER_SIZE_DEFAULT, // Tries to use the most appropriate size, defaults to large
    RENDER_SIZE_LARGE,
    RENDER_SIZE_SMALL
};

// Icon type indices (enum for better readability and maintenance)
// This enum MUST be kept in sync with any arrays that map icon types to XBM data.
enum IconType {
    ICON_TOOLS = 0,
    ICON_GAMES,
    ICON_SETTINGS,
    ICON_INFO,
    ICON_GAME_SNAKE,
    ICON_GAME_TETRIS,
    ICON_GAME_PONG,
    ICON_GAME_MAZE,
    ICON_NAV_BACK,
    ICON_NET_WIFI,
    ICON_NET_BLUETOOTH,
    ICON_TOOL_JAMMING,
    ICON_TOOL_INJECTION,
    ICON_SETTING_DISPLAY,
    ICON_SETTING_SOUND,
    ICON_SETTING_SYSTEM,
    ICON_UI_REFRESH,
    ICON_UI_CHARGING_BOLT,
    ICON_UI_VIBRATION,
    ICON_UI_LASER,
    ICON_UI_FLASHLIGHT,
    ICON_UTILITIES_CATEGORY,
    // Add new icon types here
    NUM_ICON_TYPES // Total number of icon types
};

// --- Extern Declarations for XBM Icon Data ---
// All icons will have a "large" (default 15x15) version.
// These will be defined in icons.cpp and stored in PROGMEM.

extern const unsigned char icon_tools_large_bits[];
extern const unsigned char icon_games_large_bits[];
extern const unsigned char icon_settings_large_bits[];
extern const unsigned char icon_info_large_bits[];
extern const unsigned char icon_game_snake_large_bits[];
extern const unsigned char icon_game_tetris_large_bits[];
extern const unsigned char icon_game_pong_large_bits[];
extern const unsigned char icon_game_maze_large_bits[];
extern const unsigned char icon_nav_back_large_bits[];
extern const unsigned char icon_net_wifi_large_bits[];
extern const unsigned char icon_net_bluetooth_large_bits[];
extern const unsigned char icon_tool_jamming_large_bits[];
extern const unsigned char icon_tool_injection_large_bits[];
extern const unsigned char icon_setting_display_large_bits[];
extern const unsigned char icon_setting_sound_large_bits[];
extern const unsigned char icon_setting_system_large_bits[];
extern const unsigned char icon_ui_refresh_large_bits[];
extern const unsigned char icon_ui_charging_bolt_large_bits[]; // Master large version
extern const unsigned char icon_ui_vibration_large_bits[];
extern const unsigned char icon_ui_laser_large_bits[];
extern const unsigned char icon_ui_flashlight_large_bits[];
extern const unsigned char icon_utilities_category_large_bits[];

// Only declare small versions for icons that actually have them.
extern const unsigned char icon_ui_charging_bolt_small_bits[];
extern const unsigned char icon_ui_refresh_small_bits[];
extern const unsigned char icon_nav_back_small_bits[];
// extern const unsigned char icon_another_specific_small_bits[]; // Example

#endif // ICONS_H