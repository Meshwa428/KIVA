#ifndef EVENT_H
#define EVENT_H

#include "Config.h" // For InputEvent and MenuType
#include "WifiManager.h" // For WifiNetworkInfo
#include <string>

// --- Event Types ---
enum class EventType {
    // Input System
    INPUT,

    // Navigation System
    NAVIGATE_TO_MENU,
    NAVIGATE_BACK,

    // WiFi Service
    WIFI_SCAN_REQUESTED,
    WIFI_SCAN_COMPLETED,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,

    // Jammer Service
    JAMMER_START_REQUESTED,
    JAMMER_STOPPED,

    // Song Playback
    SONG_FINISHED
};

// --- Base Event Struct ---
struct Event {
    EventType type;
    virtual ~Event() = default;
};

// --- Specific Event Payloads ---
struct InputEventData : public Event {
    InputEvent input;
    InputEventData(InputEvent ev) : input(ev) { type = EventType::INPUT; }
};

struct NavigateToMenuEvent : public Event {
    MenuType menuType;
    bool isForwardNav;
    NavigateToMenuEvent(MenuType mt, bool isFwd = true) : menuType(mt), isForwardNav(isFwd) { type = EventType::NAVIGATE_TO_MENU; }
};

#endif // EVENT_H
