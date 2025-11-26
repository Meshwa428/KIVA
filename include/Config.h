#ifndef CONFIG_H
#define CONFIG_H

#include <U8g2lib.h>
#include <cstdint> // For uint8_t

// Menu System Enums
enum class MenuType
{
    NONE,
    BACK,
    POPUP,

    // --- Main Navigation ---
    MAIN,
    TOOLS_CAROUSEL,
    GAMES_CAROUSEL,

    // --- GAME LOBBY MENUS ---
    SNAKE_GAME,

    // --- Tools Sub-menus ---
    WIFI_TOOLS_GRID,
    BLE_TOOLS_GRID,
    NRF_JAMMER_GRID,
    HOST_OTHER_GRID,

    WIFI_ATTACKS_LIST,
    WIFI_SNIFF_LIST,
    BLE_ATTACKS_LIST,

    BEACON_MODE_GRID,
    DEAUTH_MODE_GRID,
    PROBE_FLOOD_MODE_GRID,
    ASSOCIATION_SLEEP_MODES_GRID,
    BAD_MSG_MODES_GRID,

    // --- Core Attack/Tool Screens ---
    WIFI_LIST, // Re-used for multiple purposes
    BEACON_FILE_LIST,
    PORTAL_LIST,
    STATION_LIST,
    DUCKY_SCRIPT_LIST,
    CHANNEL_SELECTION,
    HANDSHAKE_CAPTURE_MENU,
    STATION_SNIFF_SAVE_ACTIVE,
    STATION_FILE_LIST,

    BEACON_SPAM_ACTIVE,
    DEAUTH_ACTIVE,
    EVIL_PORTAL_ACTIVE,
    PROBE_FLOOD_ACTIVE,
    KARMA_ACTIVE,
    PROBE_ACTIVE, // Probe Sniffer
    HANDSHAKE_CAPTURE_ACTIVE,
    BLE_SPAM_ACTIVE,
    DUCKY_SCRIPT_ACTIVE,
    JAMMING_ACTIVE,
    ASSOCIATION_SLEEP_ACTIVE,
    BAD_MSG_ACTIVE,

    // --- Settings Sub-menus ---
    SETTINGS_GRID,
    UI_SETTINGS_LIST,
    HARDWARE_SETTINGS_LIST,
    CONNECTIVITY_SETTINGS_LIST,
    SYSTEM_SETTINGS_LIST,
    BRIGHTNESS_MENU,
    TIMEZONE_LIST,

    // --- Firmware Update Menus ---
    FIRMWARE_UPDATE_GRID,
    FIRMWARE_LIST_SD,
    OTA_STATUS,

    // --- Utilities/Misc ---
    USB_DRIVE_MODE,
    TEXT_INPUT,
    WIFI_CONNECTION_STATUS,
    MUSIC_LIBRARY,
    SONG_LIST,
    NOW_PLAYING,
    INFO_MENU,

    AIR_MOUSE_MODE_GRID,
    AIR_MOUSE_ACTIVE
};

enum class SecondaryWidgetType {
    WIDGET_RAM,
    WIDGET_PSRAM,
    WIDGET_SD,
    WIDGET_CPU,
    WIDGET_TEMP
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
    BTN_RIGHT_DOWN_PRESS,

    // --- RELEASE events ---
    BTN_ENCODER_RELEASE,
    BTN_OK_RELEASE,
    BTN_BACK_RELEASE,
    BTN_UP_RELEASE,
    BTN_DOWN_RELEASE,
    BTN_LEFT_RELEASE,
    BTN_RIGHT_RELEASE,
    BTN_A_RELEASE,
    BTN_B_RELEASE,
    BTN_AI_RELEASE,
    BTN_RIGHT_UP_RELEASE,
    BTN_RIGHT_DOWN_RELEASE
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

    // --- NEW: Interrupt Pin for PCF Modules ---
    static constexpr uint8_t BTN_INTERRUPT_PIN = 42;

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

// --- Centralized Channel Definitions ---
namespace Channels {
    // The optimal channel hopping order for 2.4GHz WiFi
    static constexpr int WIFI_2_4GHZ[] = {1, 6, 11, 2, 7, 3, 8, 4, 9, 5, 10, 12, 13};
    static constexpr size_t WIFI_2_4GHZ_COUNT = sizeof(WIFI_2_4GHZ) / sizeof(int);
}

// --- Centralized 802.11 Raw Frame Definitions ---
namespace RawFrames {

    namespace Mgmt { // Management Frames
        namespace Deauth {
            static constexpr uint8_t TEMPLATE[26] = {
                0xc0, 0x00, 0x3a, 0x01,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0xf0, 0xff, 0x02, 0x00
            };
        }

        namespace Beacon {
            static constexpr uint8_t OPEN_TEMPLATE[37] = {
                0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x01, 0x02, 0x03, 0x04,
                0x05, 0x06, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                0x06, 0x07, 0x64, 0x00, 0x01, 0x04, 0x00
            };
            static constexpr uint8_t OPEN_POST_SSID_TAGS[13] = {
                0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,
                0x03, 0x01, 0x01
            };
            static constexpr uint8_t WPA2_TEMPLATE[37] = {
                0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x01, 0x02, 0x03, 0x04,
                0x05, 0x06, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                0x06, 0x07, 0x64, 0x00, 0x31, 0x04, 0x00
            };
            static constexpr uint8_t WPA2_POST_SSID_TAGS[25] = {
                0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,
                0x03, 0x01, 0x01, 0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac,
                0x04, 0x01, 0x00, 0x00, 0x0f
            };
        }

        namespace ProbeRequest {
            static constexpr uint8_t TEMPLATE[24] = {
                0x40, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0x00, 0x00
            };
            static constexpr uint8_t POST_SSID_TAGS[12] = {
                0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,
                0x32, 0x04
            };
        }

        // --- Add the Association Request frame here ---
        namespace AssociationRequest {
            // A minimal frame for the association sleep attack
            static constexpr uint8_t TEMPLATE[30] = {
                0x00, 0x10, // Frame Control (Association Request) PM=1
                0x3a, 0x01, // Duration
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (Broadcast)
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (Fake Source or BSSID)
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
                0x00, 0x00,                         // Sequence Control
                0x31, 0x00,                         // Capability Information (PM=1)
                0x0a, 0x00,                         // Listen Interval
                0x00,                               // SSID tag
                0x00,                               // SSID length      
            };
        }

        // --- NEW: Add the Bad Message EAPOL frame ---
        namespace BadMsg {
            static constexpr uint8_t EAPOL_TEMPLATE[153] = {
                0x08, 0x02,                         // Frame Control (EAPOL)
                0x00, 0x00,                         // Duration
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (Broadcast)
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (BSSID)
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
                0x30, 0x00,                         // Sequence Control
                /* LLC / SNAP */
                0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00,
                0x88, 0x8e,                          // Ethertype = EAPOL
                /* -------- 802.1X Header -------- */
                0x02,                               // Version 802.1X‑2004
                0x03,                               // Type Key
                0x00, 0x75,                          // Length 117 bytes
                /* -------- EAPOL‑Key frame body (117 B) -------- */
                0x02,                               // Desc Type 2 (AES/CCMP)
                0x00, 0xCA,                          // Key Info (Install|Ack…)
                0x00, 0x10,                          // Key Length = 16
                /* Replay Counter (8) */
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                /* Nonce (32) */
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
                /* Key IV (16) */
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                /* Key RSC (8) */
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                /* Key ID  (8) */ 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                /* Key MIC (16) */ 
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                /* Key Data Len (2) */ 
                0x00, 0x16,
                /* Key Data (22 B) */
                0xDD, 0x14,                // Vendor‑specific (PMKID IE)
                0x00, 0x0F, 0xAC, 0x04,      // OUI + Type (PMKID)
                /* PMKID (16 byte zero) */
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 
                0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11
            };
        } // namespace BadMsg
    } // namespace Mgmt

    namespace Jamming {
        // Garbage payload for Noise Injection attacks
        static constexpr char NOISE_PAYLOAD[] = "xxxxxxxxxxxxxxxx";
    }

}

// --- Centralized SD Card Path Definitions ---
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

    static constexpr const char *DATA_GAMES = "/data/games";
    static constexpr const char *DATA_LOGS = "/data/logs";
    static constexpr const char *DATA_CAPTURES = "/data/captures";
    static constexpr const char *DATA_CAPTURES_STATION_LISTS = "/data/captures/station_lists"; 
    static constexpr const char *DATA_PROBES = "/data/captures/probes";
    static constexpr const char *DATA_PROBES_SSID_SESSION = "/data/captures/probes/probes_session.txt";
    static constexpr const char *DATA_PROBES_SSID_CUMULATIVE = "/data/captures/probes/probes_cumulative.txt";
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