#ifndef EVENT_H
#define EVENT_H

#include "Config.h" // For InputEvent and MenuType

#include <string>

// --- Event Types ---
enum class EventType {
    // Input System
    APP_INPUT, // Renamed from INPUT to avoid macro collision with Arduino framework

    // Navigation System
    NAVIGATE_TO_MENU,
    NAVIGATE_BACK,
    RETURN_TO_MENU,
    REPLACE_MENU,

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
    InputEventData(InputEvent ev) : input(ev) { type = EventType::APP_INPUT; }
};

struct NavigateToMenuEvent : public Event {
    MenuType menuType;
    bool isForwardNav;
    NavigateToMenuEvent(MenuType mt, bool isFwd = true) : menuType(mt), isForwardNav(isFwd) { type = EventType::NAVIGATE_TO_MENU; }
};

struct NavigateBackEvent : public Event {
    NavigateBackEvent() { type = EventType::NAVIGATE_BACK; }
};

struct ReturnToMenuEvent : public Event {
    MenuType menuType;
    ReturnToMenuEvent(MenuType mt) : menuType(mt) { type = EventType::RETURN_TO_MENU; }
};

struct ReplaceMenuEvent : public Event {
    MenuType menuType;
    ReplaceMenuEvent(MenuType mt) : menuType(mt) { type = EventType::REPLACE_MENU; }
};

#endif // EVENT_H
