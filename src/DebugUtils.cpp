#include "DebugUtils.h"

namespace DebugUtils {

const char* menuTypeToString(MenuType type) {
    switch (type) {
        case MenuType::NONE: return "NONE";
        case MenuType::BACK: return "BACK";
        case MenuType::MAIN: return "MAIN";
        case MenuType::POPUP: return "POPUP";
        case MenuType::TOOLS_CAROUSEL: return "TOOLS_CAROUSEL";
        case MenuType::GAMES_CAROUSEL: return "GAMES_CAROUSEL";
        case MenuType::SETTINGS_CAROUSEL: return "SETTINGS_CAROUSEL";
        case MenuType::UTILITIES_CAROUSEL: return "UTILITIES_CAROUSEL";
        case MenuType::WIFI_TOOLS_GRID: return "WIFI_TOOLS_GRID";
        case MenuType::JAMMING_TOOLS_GRID: return "JAMMING_TOOLS_GRID";
        case MenuType::WIFI_LIST: return "WIFI_LIST";
        case MenuType::TEXT_INPUT: return "TEXT_INPUT";
        case MenuType::WIFI_CONNECTION_STATUS: return "WIFI_CONNECTION_STATUS";
        case MenuType::FIRMWARE_UPDATE_GRID: return "FIRMWARE_UPDATE_GRID";
        case MenuType::FIRMWARE_LIST_SD: return "FIRMWARE_LIST_SD";
        case MenuType::OTA_STATUS: return "OTA_STATUS";
        case MenuType::CHANNEL_SELECTION: return "CHANNEL_SELECTION";
        case MenuType::JAMMING_ACTIVE: return "JAMMING_ACTIVE";
        case MenuType::BEACON_MODE_SELECTION: return "BEACON_MODE_SELECTION";
        case MenuType::BEACON_FILE_LIST: return "BEACON_FILE_LIST";
        case MenuType::BEACON_SPAM_ACTIVE: return "BEACON_SPAM_ACTIVE";
        case MenuType::DEAUTH_TOOLS_GRID: return "DEAUTH_TOOLS_GRID";
        case MenuType::DEAUTH_ACTIVE: return "DEAUTH_ACTIVE";
        case MenuType::EVIL_TWIN_PORTAL_SELECTION: return "EVIL_TWIN_PORTAL_SELECTION";
        case MenuType::EVIL_TWIN_ACTIVE: return "EVIL_TWIN_ACTIVE";
        case MenuType::PROBE_ACTIVE: return "PROBE_ACTIVE";
        case MenuType::KARMA_ACTIVE: return "KARMA_ACTIVE";
        case MenuType::BLE_TOOLS_GRID: return "BLE_TOOLS_GRID";
        case MenuType::HOST_TOOLS_GRID: return "HOST_TOOLS_GRID";
        case MenuType::BLE_SPAM_ACTIVE: return "BLE_SPAM_ACTIVE";
        case MenuType::DUCKY_SCRIPT_LIST: return "DUCKY_SCRIPT_LIST";
        case MenuType::DUCKY_SCRIPT_ACTIVE: return "DUCKY_SCRIPT_ACTIVE";
        case MenuType::BLE_KEYBOARD_MAPPER_ACTIVE: return "BLE_KEYBOARD_MAPPER_ACTIVE";
        case MenuType::BLE_MEDIA_CONTROLLER_ACTIVE: return "BLE_MEDIA_CONTROLLER_ACTIVE";
        case MenuType::BLE_SCROLL_HELPER_ACTIVE: return "BLE_SCROLL_HELPER_ACTIVE";
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

} // namespace DebugUtils