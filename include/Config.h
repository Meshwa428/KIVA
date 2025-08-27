#ifndef CONFIG_H
#define CONFIG_H

#include <U8g2lib.h>
#include <cstdint> // For uint8_t

// Menu System Enums
enum class MenuType
{
    NONE,
    BACK,
    MAIN,
    POPUP,
    TOOLS_CAROUSEL,
    GAMES_CAROUSEL,
    SETTINGS,
    BRIGHTNESS_MENU,
    UTILITIES_CAROUSEL,
    WIFI_TOOLS_GRID,
    JAMMING_TOOLS_GRID,
    WIFI_LIST,
    TEXT_INPUT,
    WIFI_CONNECTION_STATUS,
    FIRMWARE_UPDATE_GRID,
    FIRMWARE_LIST_SD,
    OTA_STATUS,
    CHANNEL_SELECTION,
    JAMMING_ACTIVE,
    BEACON_MODE_SELECTION,
    BEACON_FILE_LIST,
    BEACON_SPAM_ACTIVE,
    DEAUTH_TOOLS_GRID,
    DEAUTH_ACTIVE,
    EVIL_TWIN_PORTAL_SELECTION,
    EVIL_TWIN_ACTIVE,
    PROBE_ACTIVE,
    KARMA_ACTIVE,
    BLE_TOOLS_GRID,
    HOST_TOOLS_GRID,
    BLE_SPAM_ACTIVE,
    DUCKY_SCRIPT_LIST,
    DUCKY_SCRIPT_ACTIVE,
    BLE_KEYBOARD_MAPPER_ACTIVE,
    BLE_MEDIA_CONTROLLER_ACTIVE,
    BLE_SCROLL_HELPER_ACTIVE,
    USB_DRIVE_MODE,
    HANDSHAKE_CAPTURE_MENU,
    HANDSHAKE_CAPTURE_ACTIVE,
    // --- NEW MUSIC PLAYER MENU TYPES ---
    MUSIC_PLAYER_LIST,
    NOW_PLAYING
};

// Input Event System
enum class InputEvent
{
    NONE,
    ENCODER_CW,
    ENCODER_CCW,
    BTN_ENCODER_PRESS,
    BTN_OK_PRESS,
    BTN_BACK_PRESS,
    BTN_UP_PRESS,
    BTN_DOWN_PRESS,
    BTN_LEFT_PRESS,
    BTN_RIGHT_PRESS,
    BTN_A_PRESS,
    BTN_B_PRESS,
    BTN_AI_PRESS,
    BTN_RIGHT_UP_PRESS,
    BTN_RIGHT_DOWN_PRESS
};

// I2C & Hardware Pins
namespace Pins
{
    static constexpr uint8_t MUX_ADDR = 0x70;
    static constexpr uint8_t PCF0_ADDR = 0x24;
    static constexpr uint8_t PCF1_ADDR = 0x20;
    static constexpr uint8_t MUX_CHANNEL_PCF0_ENCODER = 0;
    static constexpr uint8_t MUX_CHANNEL_PCF1_NAV = 1;
    static constexpr uint8_t MUX_CHANNEL_MAIN_DISPLAY = 4;
    static constexpr uint8_t MUX_CHANNEL_SECOND_DISPLAY = 2;
    static constexpr uint8_t MUX_CHANNEL_MPU = 3;
    static constexpr uint8_t MUX_CHANNEL_RTC = 5;
    static constexpr uint8_t MOSI_PIN = 8;
    static constexpr uint8_t SCK_PIN = 7;
    static constexpr uint8_t MISO_PIN = 9;

    static constexpr uint8_t ENC_BTN = 0;
    static constexpr uint8_t ENC_A = 1;
    static constexpr uint8_t ENC_B = 2;
    static constexpr uint8_t AI_BTN = 3;
    static constexpr uint8_t RIGHT_UP = 4;
    static constexpr uint8_t RIGHT_DOWN = 5;
    static constexpr uint8_t LASER_PIN_PCF0 = 6;
    static constexpr uint8_t MOTOR_PIN_PCF0 = 7;

    static constexpr uint8_t NAV_OK = 0;
    static constexpr uint8_t NAV_BACK = 1;
    static constexpr uint8_t NAV_A = 2;
    static constexpr uint8_t NAV_B = 3;
    static constexpr uint8_t NAV_LEFT = 4;
    static constexpr uint8_t NAV_UP = 5;
    static constexpr uint8_t NAV_DOWN = 6;
    static constexpr uint8_t NAV_RIGHT = 7;

    static constexpr uint8_t ADC_PIN = 3;

    static constexpr uint8_t SD_CS_PIN = 21;
    static constexpr uint8_t AMPLIFIER_PIN = 4;

    static constexpr uint8_t NRF1_CE_PIN = 1;
    static constexpr uint8_t NRF1_CSN_PIN = 2;
    static constexpr uint8_t NRF2_CE_PIN = 43;
    static constexpr uint8_t NRF2_CSN_PIN = 44;
}

// Battery Monitor Constants
namespace Battery
{
    static constexpr float ADC_REF_VOLTAGE = 3.3f;
    static constexpr int ADC_RESOLUTION = 4095;
    static constexpr float VOLTAGE_DIVIDER_RATIO = 1.50f; // (R1+R2)/R2
    static constexpr int NUM_SAMPLES = 20;
    static constexpr unsigned long CHECK_INTERVAL_MS = 1000;
}

// --- NEW: Centralized SD Card Path Definitions ---
namespace SD_ROOT
{
    // Top-level directories
    static constexpr const char *CONFIG = "/config";
    static constexpr const char *DATA = "/data";
    static constexpr const char *USER = "/user";
    static constexpr const char *FIRMWARE = "/firmware";
    static constexpr const char *WEB = "/web";

    // Second-level and specific file paths
    static constexpr const char *WIFI_KNOWN_NETWORKS = "/config/wifi_known_networks.txt";
    static constexpr const char *CONFIG_CURRENT_FIRMWARE = "/config/current_firmware.json";

    static constexpr const char *DATA_LOGS = "/data/logs";
    static constexpr const char *DATA_CAPTURES = "/data/captures";
    static constexpr const char *DATA_PROBES = "/data/captures/probes";
    static constexpr const char *CAPTURES_EVILTWIN_CSV = "/data/captures/evil_twin_credentials.csv";

    static constexpr const char *USER_BEACON_LISTS = "/user/beacon_lists";
    static constexpr const char *USER_PORTALS = "/user/portals";
    static constexpr const char *USER_DUCKY = "/user/ducky_scripts";
    static constexpr const char *USER_MUSIC = "/user/music";

    static constexpr const char *WEB_OTA_PAGE = "/web/ota_page.html";

    // Log file info
    static constexpr const char *LOG_BASE_NAME = "system_log_";
}

// Firmware & OTA Constants
namespace Firmware
{
    static constexpr const char* OTA_HOSTNAME = "kiva-device";
    static constexpr const char* METADATA_EXTENSION = ".kfw";
    static constexpr int MAX_FIRMWARES_ON_SD = 10;
    static constexpr int MIN_AP_PASSWORD_LEN = 8;
    static constexpr int MAX_AP_PASSWORD_LEN = 63;
}

// Debouncing & Auto-repeat
static constexpr unsigned long DEBOUNCE_DELAY_MS = 50;
static constexpr unsigned long REPEAT_INIT_DELAY_MS = 400; // Time to hold before first repeat
static constexpr unsigned long REPEAT_INTERVAL_MS = 150;   // Time between subsequent repeats

// UI Layout Constants
static constexpr int STATUS_BAR_H = 11;
static constexpr float GRID_ANIM_STAGGER_DELAY = 40.0f;
static constexpr float GRID_ANIM_SPEED = 20.0f;

#endif // CONFIG_H