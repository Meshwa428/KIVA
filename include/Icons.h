#ifndef ICONS_H
#define ICONS_H

#include <cstdint>

// Icon Dimensions
namespace IconSize
{
    static constexpr int LARGE_WIDTH = 15;
    static constexpr int LARGE_HEIGHT = 15;
    static constexpr int SMALL_WIDTH = 7;
    static constexpr int SMALL_HEIGHT = 7;
    static constexpr int BOOT_LOGO_WIDTH = 128;
    static constexpr int BOOT_LOGO_HEIGHT = 64;
}

// Icon type indices
enum class IconType
{
    TOOLS,
    GAMES,
    SETTINGS,
    INFO,
    ERROR,
    GAME_SNAKE,
    GAME_TETRIS,
    GAME_PONG,
    GAME_MAZE,
    NAV_BACK,
    NET_WIFI,
    NET_BLUETOOTH,
    TOOL_JAMMING,
    TOOL_INJECTION,
    TOOL_PROBE,
    SETTING_DISPLAY,
    SETTING_SOUND,
    SETTING_SYSTEM,
    UI_REFRESH,
    UI_CHARGING_BOLT,
    UI_VIBRATION,
    UI_LASER,
    UI_FLASHLIGHT,
    UTILITIES_CATEGORY,
    BOOT_LOGO,
    NET_WIFI_LOCK,
    SD_CARD,
    BEACON,
    SKULL,
    BASIC_OTA,
    FIRMWARE_UPDATE,
    DISCONNECT,
    USB,
    TARGET,
    // --- NEW MUSIC ICONS ---
    MUSIC_PLAYER,
    PLAYLIST,
    MUSIC_NOTE,
    PLAY,
    PAUSE,
    NEXT_TRACK,
    PREV_TRACK,
    SHUFFLE,
    REPEAT,
    REPEAT_ONE,
    NONE,
    NUM_ICON_TYPES
};

extern const unsigned char icon_boot_logo_bits[];

// Extern Declarations for XBM Icon Data
extern const unsigned char icon_none_large_bits[];
extern const unsigned char icon_tools_large_bits[];
extern const unsigned char icon_games_large_bits[];
extern const unsigned char icon_settings_large_bits[];
extern const unsigned char icon_info_large_bits[];
extern const unsigned char icon_error_large_bits[];
extern const unsigned char icon_game_snake_large_bits[];
extern const unsigned char icon_game_tetris_large_bits[];
extern const unsigned char icon_game_pong_large_bits[];
extern const unsigned char icon_game_maze_large_bits[];
extern const unsigned char icon_nav_back_large_bits[];
extern const unsigned char icon_net_wifi_large_bits[];
extern const unsigned char icon_net_bluetooth_large_bits[];
extern const unsigned char icon_tool_jamming_large_bits[];
extern const unsigned char icon_tool_injection_large_bits[];
extern const unsigned char icon_probe_request_large_bits[];
extern const unsigned char icon_setting_display_large_bits[];
extern const unsigned char icon_setting_sound_large_bits[];
extern const unsigned char icon_setting_system_large_bits[];
extern const unsigned char icon_ui_refresh_large_bits[];
extern const unsigned char icon_ui_charging_bolt_large_bits[];
extern const unsigned char icon_ui_vibration_large_bits[];
extern const unsigned char icon_ui_laser_large_bits[];
extern const unsigned char icon_ui_flashlight_large_bits[];
extern const unsigned char icon_utilities_category_large_bits[];
extern const unsigned char icon_sd_card_large_bits[];
extern const unsigned char icon_beacon_large_bits[];
extern const unsigned char icon_skull_large_bits[];
extern const unsigned char icon_basic_ota_large_bits[];
extern const unsigned char icon_firmware_update_large_bits[];
extern const unsigned char icon_disconnect_large_bits[];
extern const unsigned char icon_usb_large_bits[];
extern const unsigned char icon_target_large_bits[];
// --- NEW MUSIC ICONS ---
extern const unsigned char icon_music_player_large_bits[];
extern const unsigned char icon_playlist_large_bits[];
extern const unsigned char icon_music_note_large_bits[];
extern const unsigned char icon_play_large_bits[];
extern const unsigned char icon_pause_large_bits[];
extern const unsigned char icon_next_track_large_bits[];
extern const unsigned char icon_prev_track_large_bits[];
extern const unsigned char icon_shuffle_large_bits[];
extern const unsigned char icon_repeat_large_bits[];
extern const unsigned char icon_repeat_one_large_bits[];

// Small Icons
extern const unsigned char icon_ui_charging_bolt_small_bits[];
extern const unsigned char icon_ui_refresh_small_bits[];
extern const unsigned char icon_nav_back_small_bits[];
extern const unsigned char icon_lock_small_bits[];

#endif // ICONS_H