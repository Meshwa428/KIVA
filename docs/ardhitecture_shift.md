1.  **Button inputs are paramount**, and while the interrupt was a good *hardware optimization* to reduce CPU load from polling I2C, it didn't fundamentally change the *software architecture*, which is still poll-based.
2.  The project **lacks strong abstraction**. The `App` class has become a "God Object"—a common anti-pattern where one class knows about and controls almost everything else. This makes the system rigid, hard to test, and difficult to reason about.
3.  The execution model is imperative and poll-based (`if (event) { do_thing; }` inside `loop()`). Your desire for a more "deterministic" and "automatic" system points towards a more declarative, **event-driven architecture**.

Let's design a better architecture. This is a significant refactoring, but it will result in a vastly more robust, scalable, and professional system.

### The Core Problem: The Monolithic `App` Class

Currently, `App` holds an instance of every single manager and every single menu. Menus get a pointer to `App` and call `app->getJammer().start()`. This creates tight coupling:
*   The `JammingActiveMenu` *must know* that `App` has a `Jammer` object.
*   The `App` class *must know* about every single menu to register it.
*   The main `loop()` polls every single manager (`jammer_.loop()`, `wifiManager_.update()`, etc.), even if they are idle.

## Proposed Architecture: A Decoupled, Event-Driven System

We will rebuild the core around three key concepts:

1.  **An Event Dispatcher (or "Event Bus"):** A central singleton that delivers messages (Events) from a "publisher" to any number of "subscribers" without them knowing about each other.
2.  **A True State Machine for UI:** A dedicated class that manages the `navigationStack_` and responds to navigation events, cleaning up the logic in `App`.
3.  **Independent Services:** Managers like `Jammer`, `WifiManager`, etc., will become services that listen for events and publish their own status events. They will no longer be polled directly in the main loop unless they are active.

---

### Step 1: The Event System

First, we need a way for components to communicate without direct pointers.

**`KIVA/include/Event.h` (New File)**

```h
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
```

**`KIVA/include/EventDispatcher.h` (New File)**

```h
#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H

#include "Event.h"
#include <vector>
#include <map>
#include <functional>

// An interface for any class that wants to listen to events
class ISubscriber {
public:
    virtual ~ISubscriber() = default;
    virtual void onEvent(const Event& event) = 0;
};

class EventDispatcher {
public:
    static EventDispatcher& getInstance();

    void subscribe(EventType type, ISubscriber* subscriber);
    void unsubscribe(EventType type, ISubscriber* subscriber);
    void publish(const Event& event);

    // Disable copy/assignment
    EventDispatcher(const EventDispatcher&) = delete;
    void operator=(const EventDispatcher&) = delete;

private:
    EventDispatcher() = default;
    std::map<EventType, std::vector<ISubscriber*>> subscribers_;
};

#endif // EVENT_DISPATCHER_H
```

**`KIVA/src/EventDispatcher.cpp` (New File)**

```cpp
#include "EventDispatcher.h"
#include <algorithm>

EventDispatcher& EventDispatcher::getInstance() {
    static EventDispatcher instance;
    return instance;
}

void EventDispatcher::subscribe(EventType type, ISubscriber* subscriber) {
    subscribers_[type].push_back(subscriber);
}

void EventDispatcher::unsubscribe(EventType type, ISubscriber* subscriber) {
    auto& subs = subscribers_[type];
    subs.erase(std::remove(subs.begin(), subs.end(), subscriber), subs.end());
}

void EventDispatcher::publish(const Event& event) {
    if (subscribers_.count(event.type)) {
        for (auto* subscriber : subscribers_[event.type]) {
            subscriber->onEvent(event);
        }
    }
}
```

---

### Step 2: Refactor `HardwareManager` to Publish Events

Now, `HardwareManager`'s job isn't to queue inputs, but to publish them. It becomes a pure hardware abstraction layer.

**`KIVA/src/HardwareManager.cpp` (Modified `update` and `processButtons_...`)**

```cpp
// In HardwareManager.cpp

#include "HardwareManager.h"
#include "EventDispatcher.h" // Include the new dispatcher
#include <Arduino.h>
// ... other includes ...

// The static instance and ISR remain the same.

void HardwareManager::update()
{
    // Time-based events run continuously
    if (millis() - lastBatteryCheckTime_ >= Battery::CHECK_INTERVAL_MS) {
        updateBattery();
    }
    processButtonRepeats();

    // Interrupt-driven events
    if (buttonInterruptFired_) {
        // Read from PCFs to clear the interrupt
        uint8_t pcf0State = 0xFF, pcf1State = 0xFF;
        
        selectMux(Pins::MUX_CHANNEL_PCF0_ENCODER);
        pcf0State = readPCF(Pins::PCF0_ADDR);
        
        selectMux(Pins::MUX_CHANNEL_PCF1_NAV);
        pcf1State = readPCF(Pins::PCF1_ADDR);
        
        // Process the data we just read
        processEncoder(pcf0State); // Pass state to avoid re-reading
        processButton_PCF0(pcf0State);
        processButtons_PCF1(pcf1State);
        
        buttonInterruptFired_ = false;
    }
}

// Key change: processEncoder and processButtons no longer poll.
// They now take the state read in update() as an argument.
// And instead of pushing to a queue, they PUBLISH an event.

void HardwareManager::processButton_PCF0(uint8_t pcf0State)
{
    // ... debouncing logic ...
    
    // if a press is detected:
    // OLD: inputQueue_.push_back(InputEvent::BTN_ENCODER_PRESS);
    // NEW:
    EventDispatcher::getInstance().publish(InputEventData(InputEvent::BTN_ENCODER_PRESS));
}

void HardwareManager::processButtons_PCF1(uint8_t pcf1State)
{
    // ... debouncing logic for all buttons ...

    // For each button press detected:
    // OLD: inputQueue_.push_back(mapPcf1PinToEvent(pin));
    // NEW:
    EventDispatcher::getInstance().publish(InputEventData(mapPcf1PinToEvent(pin)));
}

// The getNextInputEvent() method can now be completely removed from HardwareManager.
```

The key takeaway is that `HardwareManager` is now **dumb**. It doesn't know what a menu is or who is listening. It just announces hardware state changes to the entire system.

---

### Step 3: Refactor Menus to be Subscribers

Menus no longer have a `handleInput` method that gets called in a loop. Instead, they subscribe to the `INPUT` event type when they become active.

**`KIVA/include/IMenu.h` (Modified)**

```h
#ifndef I_MENU_H
#define I_MENU_H

#include "Config.h"
#include "Icons.h"
#include "EventDispatcher.h" // Include the dispatcher
#include <vector>
#include <string>
#include <functional> // For std::function

// ... MenuItem struct is unchanged ...

// A menu is now also an ISubscriber
class IMenu : public ISubscriber {
public:
    virtual ~IMenu() {}

    // Lifecycle methods
    virtual void onEnter(App* app, bool isForwardNav) = 0;
    virtual void onUpdate(App* app) = 0;
    virtual void onExit(App* app) = 0;
    virtual void draw(App* app, U8G2& display) = 0;

    // The onEvent method from ISubscriber interface
    void onEvent(const Event& event) override;

    // REMOVED: virtual void handleInput(App* app, InputEvent event) = 0;
    
    // ... other methods are unchanged ...
protected:
    // A new helper method for menus to implement
    virtual void handleInput(InputEvent event, App* app) = 0;
};

#endif // I_MENU_H
```

**`KIVA/src/MainMenu.cpp` (Example Refactor)**

```cpp
#include "MainMenu.h"
#include "App.h"
#include "EventDispatcher.h" // For subscribing
#include "Event.h"           // For event types and data

// ... constructor is unchanged ...

void MainMenu::onEnter(App* app, bool isForwardNav) {
    // When the menu becomes active, it subscribes to input events.
    EventDispatcher::getInstance().subscribe(EventType::INPUT, this);

    // ... rest of onEnter logic (animation setup) ...
}

void MainMenu::onExit(App* app) {
    // When the menu is no longer active, it unsubscribes.
    // This is CRITICAL for preventing inactive menus from handling input.
    EventDispatcher::getInstance().unsubscribe(EventType::INPUT, this);
}

// The onEvent method filters events and calls the specific handler.
void IMenu::onEvent(const Event& event) {
    if (event.type == EventType::INPUT) {
        // We know App is valid because a menu can't exist without it.
        // We need a way to get the App context. For now, we assume a global or service locator.
        // Let's assume App provides a static getInstance() for simplicity here.
        const auto& inputEvent = static_cast<const InputEventData&>(event);
        handleInput(inputEvent.input, &App::getInstance()); // We'll need to implement App::getInstance()
    }
}

// The old handleInput logic moves here.
void MainMenu::handleInput(InputEvent event, App* app) {
    switch(event) {
        case InputEvent::BTN_OK_PRESS:
            // OLD: app->changeMenu(...);
            // NEW: Publish a navigation event. The UI State Machine will catch it.
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(menuItems_[selectedIndex_].targetMenu));
            break;
        // ... other cases ...
    }
}
```

This is a massive improvement. The main `App::loop()` no longer needs to fetch an event and pass it to `currentMenu_->handleInput()`. The `EventDispatcher` handles the routing automatically and deterministically. The active menu gets the event directly.

### Summary of Benefits

1.  **True Decoupling:** `HardwareManager` doesn't know about menus. Menus don't know about `HardwareManager`. The `App` class is no longer the middleman for everything.
2.  **Efficiency:** No more polling the input queue in the main loop. Code only runs when an actual interrupt-fired event is published.
3.  **Scalability:** Adding a new feature (e.g., a background task that needs to display a notification) is easy. It just publishes a `SHOW_POPUP` event. The UI manager subscribes to it and handles the logic, without the new feature needing to know anything about the UI.
4.  **Clarity and Determinism:** The flow of control is much clearer. An event happens, it's published, and the correct subscriber handles it. This is exactly the "automatic" and "deterministic" model you were looking for.
5.  **Testability:** You can now test individual components by creating a mock `EventDispatcher` and sending them specific events, without needing a full `App` instance.