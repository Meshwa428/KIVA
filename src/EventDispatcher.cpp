#include "EventDispatcher.h"
#include "DebugUtils.h" // For logging
#include "Logger.h"     // For logging
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
        // --- MODIFICATION START: Create a safe copy of the subscriber list ---
        // This prevents iterator invalidation if a handler calls subscribe() or unsubscribe().
        auto subscribers_copy = subscribers_[event.type];
        // --- MODIFICATION END ---

        // --- MODIFICATION START: Iterate over the safe copy ---
        for (auto* subscriber : subscribers_copy) {
            subscriber->onEvent(event);
        }
        // --- MODIFICATION END ---
    }
}