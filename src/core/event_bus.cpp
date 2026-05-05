#include "event_bus.h"

EventBus eventBus;

void EventBus::init() {
    count = 0;
    for (uint8_t i = 0; i < MAX_LISTENERS; i++) {
        listeners[i].active = false;
    }
}

void EventBus::subscribe(EventType type, EventCallback cb) {
    if (count >= MAX_LISTENERS) return;
    for (uint8_t i = 0; i < MAX_LISTENERS; i++) {
        if (!listeners[i].active) {
            listeners[i].type = type;
            listeners[i].callback = cb;
            listeners[i].active = true;
            count++;
            return;
        }
    }
}

void EventBus::unsubscribe(EventType type, EventCallback cb) {
    for (uint8_t i = 0; i < MAX_LISTENERS; i++) {
        if (listeners[i].active && listeners[i].type == type &&
            listeners[i].callback == cb) {
            listeners[i].active = false;
            count--;
            return;
        }
    }
}

void EventBus::publish(const EventMessage& msg) {
    for (uint8_t i = 0; i < MAX_LISTENERS; i++) {
        if (listeners[i].active && listeners[i].type == msg.type) {
            listeners[i].callback(msg);
        }
    }
}

void EventBus::clear() {
    count = 0;
    for (uint8_t i = 0; i < MAX_LISTENERS; i++) {
        listeners[i].active = false;
    }
}
