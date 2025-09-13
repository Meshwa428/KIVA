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
        for (auto* subscriber : subscribers_[event.type]) {
            subscriber->onEvent(event);
        }
    }
}
