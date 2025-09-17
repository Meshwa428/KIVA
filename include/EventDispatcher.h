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
