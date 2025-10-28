#include "DebugUtils.h"

namespace DebugUtils {

const char* menuTypeToString(MenuType type) {
    switch (type) {
        case MenuType::NONE: return "NONE";
        case MenuType::BACK: return "BACK";
        case MenuType::POPUP: return "POPUP";

        // --- Main Navigation ---
        case MenuType::MAIN: return "MAIN";
        case MenuType::TOOLS_CAROUSEL: return "TOOLS_CAROUSEL";
        case MenuType::GAMES_CAROUSEL: return "GAMES_CAROUSEL";

        // --- GAME LOBBY MENUS ---
        case MenuType::SNAKE_GAME: return "SNAKE_GAME";

        // --- Tools Sub-menus ---
        case MenuType::WIFI_TOOLS_GRID: return "WIFI_TOOLS_GRID";
        case MenuType::BLE_TOOLS_GRID: return "BLE_TOOLS_GRID";
        case MenuType::NRF_JAMMER_GRID: return "NRF_JAMMER_GRID";
        case MenuType::HOST_OTHER_GRID: return "HOST_OTHER_GRID";
        case MenuType::WIFI_ATTACKS_LIST: return "WIFI_ATTACKS_LIST";
        case MenuType::WIFI_SNIFF_LIST: return "WIFI_SNIFF_LIST";
        case MenuType::BLE_ATTACKS_LIST: return "BLE_ATTACKS_LIST";
        case MenuType::BEACON_MODE_GRID: return "BEACON_MODE_GRID";
        case MenuType::DEAUTH_MODE_GRID: return "DEAUTH_MODE_GRID";
        case MenuType::PROBE_FLOOD_MODE_GRID: return "PROBE_FLOOD_MODE_GRID";
        case MenuType::ASSOCIATION_SLEEP_MODES_GRID: return "ASSOCIATION_SLEEP_MODES_GRID";
        case MenuType::BAD_MSG_MODES_GRID: return "BAD_MSG_MODES_GRID";

        // --- Core Attack/Tool Screens ---
        case MenuType::WIFI_LIST: return "WIFI_LIST";
        case MenuType::BEACON_FILE_LIST: return "BEACON_FILE_LIST";
        case MenuType::PORTAL_LIST: return "PORTAL_LIST";
        case MenuType::STATION_LIST: return "STATION_LIST";
        case MenuType::DUCKY_SCRIPT_LIST: return "DUCKY_SCRIPT_LIST";
        case MenuType::CHANNEL_SELECTION: return "CHANNEL_SELECTION";
        case MenuType::HANDSHAKE_CAPTURE_MENU: return "HANDSHAKE_CAPTURE_MENU";
        case MenuType::STATION_SNIFF_SAVE_ACTIVE: return "STATION_SNIFF_SAVE_ACTIVE";
        case MenuType::STATION_FILE_LIST: return "STATION_FILE_LIST";

        // --- Active Screens ---
        case MenuType::BEACON_SPAM_ACTIVE: return "BEACON_SPAM_ACTIVE";
        case MenuType::DEAUTH_ACTIVE: return "DEAUTH_ACTIVE";
        case MenuType::EVIL_PORTAL_ACTIVE: return "EVIL_PORTAL_ACTIVE";
        case MenuType::PROBE_FLOOD_ACTIVE: return "PROBE_FLOOD_ACTIVE";
        case MenuType::KARMA_ACTIVE: return "KARMA_ACTIVE";
        case MenuType::PROBE_ACTIVE: return "PROBE_ACTIVE";
        case MenuType::HANDSHAKE_CAPTURE_ACTIVE: return "HANDSHAKE_CAPTURE_ACTIVE";
        case MenuType::BLE_SPAM_ACTIVE: return "BLE_SPAM_ACTIVE";
        case MenuType::DUCKY_SCRIPT_ACTIVE: return "DUCKY_SCRIPT_ACTIVE";
        case MenuType::JAMMING_ACTIVE: return "JAMMING_ACTIVE";
        case MenuType::ASSOCIATION_SLEEP_ACTIVE: return "ASSOCIATION_SLEEP_ACTIVE";
        case MenuType::BAD_MSG_ACTIVE: return "BAD_MSG_ACTIVE";

        // --- Settings Sub-menus ---
        case MenuType::SETTINGS_GRID: return "SETTINGS_GRID";
        case MenuType::UI_SETTINGS_LIST: return "UI_SETTINGS_LIST";
        case MenuType::HARDWARE_SETTINGS_LIST: return "HARDWARE_SETTINGS_LIST";
        case MenuType::CONNECTIVITY_SETTINGS_LIST: return "CONNECTIVITY_SETTINGS_LIST";
        case MenuType::SYSTEM_SETTINGS_LIST: return "SYSTEM_SETTINGS_LIST";
        case MenuType::BRIGHTNESS_MENU: return "BRIGHTNESS_MENU";
        case MenuType::TIMEZONE_LIST: return "TIMEZONE_LIST";

        // --- Firmware Update Menus ---
        case MenuType::FIRMWARE_UPDATE_GRID: return "FIRMWARE_UPDATE_GRID";
        case MenuType::FIRMWARE_LIST_SD: return "FIRMWARE_LIST_SD";
        case MenuType::OTA_STATUS: return "OTA_STATUS";

        // --- Utilities/Misc ---
        case MenuType::USB_DRIVE_MODE: return "USB_DRIVE_MODE";
        case MenuType::TEXT_INPUT: return "TEXT_INPUT";
        case MenuType::WIFI_CONNECTION_STATUS: return "WIFI_CONNECTION_STATUS";
        case MenuType::MUSIC_LIBRARY: return "MUSIC_LIBRARY";
        case MenuType::SONG_LIST: return "SONG_LIST";
        case MenuType::NOW_PLAYING: return "NOW_PLAYING";
        case MenuType::INFO_MENU: return "INFO_MENU";
        
        default: return "UNKNOWN_MENU";
    }
}

const char* inputEventToString(InputEvent event) {
    switch (event) {
        case InputEvent::NONE: return "NONE";
        case InputEvent::ENCODER_CW: return "ENCODER_CW";
        case InputEvent::ENCODER_CCW: return "ENCODER_CCW";
        case InputEvent::BTN_ENCODER_PRESS: return "BTN_ENCODER_PRESS";
        case InputEvent::BTN_OK_PRESS: return "BTN_OK_PRESS";
        case InputEvent::BTN_BACK_PRESS: return "BTN_BACK_PRESS";
        case InputEvent::BTN_UP_PRESS: return "BTN_UP_PRESS";
        case InputEvent::BTN_DOWN_PRESS: return "BTN_DOWN_PRESS";
        case InputEvent::BTN_LEFT_PRESS: return "BTN_LEFT_PRESS";
        case InputEvent::BTN_RIGHT_PRESS: return "BTN_RIGHT_PRESS";
        case InputEvent::BTN_A_PRESS: return "BTN_A_PRESS";
        case InputEvent::BTN_B_PRESS: return "BTN_B_PRESS";
        case InputEvent::BTN_AI_PRESS: return "BTN_AI_PRESS";
        case InputEvent::BTN_RIGHT_UP_PRESS: return "BTN_RIGHT_UP_PRESS";
        case InputEvent::BTN_RIGHT_DOWN_PRESS: return "BTN_RIGHT_DOWN_PRESS";
        default: return "UNKNOWN_INPUT";
    }
}

const char* muxChannelToString(uint8_t channel) {
    switch (channel) {
        case Pins::MUX_CHANNEL_PCF0_ENCODER:
            return "PCF0 (Encoder)";
        case Pins::MUX_CHANNEL_PCF1_NAV:
            return "PCF1 (Nav)";
        case Pins::MUX_CHANNEL_MAIN_DISPLAY:
            return "Main Display";
        case Pins::MUX_CHANNEL_SECOND_DISPLAY:
            return "Second Display";
        case Pins::MUX_CHANNEL_MPU:
            return "MPU";
        case Pins::MUX_CHANNEL_RTC:
            return "RTC";
        default:
            return "Unknown";
    }
}

const char* pcfStateToString(uint8_t pcfAddress, uint8_t state) {
    static char buffer[128];
    buffer[0] = '\0';
    bool first = true;

    strcat(buffer, "[");

    for (int i = 0; i < 8; ++i) {
        // A '0' bit means the pin is LOW
        if (!(state & (1 << i))) {
            if (!first) {
                strcat(buffer, ", ");
            }

            const char* btnName = "???";
            if (pcfAddress == Pins::PCF0_ADDR) {
                // --- THIS IS THE FIX ---
                // Add labels for all pins on PCF0
                switch(i) {
                    case Pins::ENC_BTN:        btnName = "ENC_BTN"; break;
                    case Pins::ENC_A:          btnName = "ENC_A"; break;
                    case Pins::ENC_B:          btnName = "ENC_B"; break;
                    case Pins::AI_BTN:         btnName = "AI_BTN"; break;
                    case Pins::RIGHT_UP:       btnName = "R_UP"; break;
                    case Pins::RIGHT_DOWN:     btnName = "R_DOWN"; break;
                    case Pins::LASER_PIN_PCF0: btnName = "LASER"; break;
                    case Pins::MOTOR_PIN_PCF0: btnName = "MOTOR"; break;
                }
            } else if (pcfAddress == Pins::PCF1_ADDR) {
                switch(i) {
                    case Pins::NAV_OK:    btnName = "OK"; break;
                    case Pins::NAV_BACK:  btnName = "BACK"; break;
                    case Pins::NAV_A:     btnName = "A"; break;
                    case Pins::NAV_B:     btnName = "B"; break;
                    case Pins::NAV_LEFT:  btnName = "LEFT"; break;
                    case Pins::NAV_UP:    btnName = "UP"; break;
                    case Pins::NAV_DOWN:  btnName = "DOWN"; break;
                    case Pins::NAV_RIGHT: btnName = "RIGHT"; break;
                }
            }
            strcat(buffer, btnName);
            first = false;
        }
    }

    if (first) {
        strcat(buffer, "None");
    }
    strcat(buffer, "]");
    return buffer;
}

} // namespace DebugUtils