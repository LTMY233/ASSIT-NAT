#include "button_mgr.h"
#include "config.h"

ButtonMgr buttonMgr;

void ButtonMgr::init(uint8_t pinUp, uint8_t pinDown, uint8_t pinOk) {
    btns[0].pin = pinUp;
    btns[1].pin = pinDown;
    btns[2].pin = pinOk;

    for (uint8_t i = 0; i < 3; i++) {
        pinMode(btns[i].pin, INPUT_PULLUP);
        btns[i].stable = HIGH;
        btns[i].lastRaw = HIGH;
        btns[i].lastChange = 0;
        btns[i].lastRelease = 0;
        btns[i].longFired = false;
    }
    queueHead = 0;
    queueTail = 0;
}

void ButtonMgr::pushEvent(ButtonEvent ev) {
    uint8_t next = (queueHead + 1) % QUEUE_SIZE;
    if (next == queueTail) return;
    eventQueue[queueHead] = ev;
    queueHead = next;
}

bool ButtonMgr::getEvent(ButtonEvent& ev) {
    if (queueTail == queueHead) return false;
    ev = eventQueue[queueTail];
    queueTail = (queueTail + 1) % QUEUE_SIZE;
    return true;
}

bool ButtonMgr::hasEvents() const {
    return queueTail != queueHead;
}

void ButtonMgr::flush() {
    queueHead = 0;
    queueTail = 0;
}

void ButtonMgr::update() {
    uint32_t now = millis();

    for (uint8_t i = 0; i < 3; i++) {
        BtnState& btn = btns[i];
        bool raw = digitalRead(btn.pin);

        // Level change -> record time
        if (raw != btn.lastRaw) {
            btn.lastChange = now;
            btn.lastRaw = raw;
            continue;  // wait debounce
        }

        // In debounce -> skip
        if ((now - btn.lastChange) < BTN_DEBOUNCE_MS) continue;

        // Stable -> detect long press / repeat
        if (raw == btn.stable) {
            if (btn.stable == LOW) {
                if (!btn.longFired) {
                    if ((now - btn.lastChange) >= BTN_LONG_PRESS_MS) {
                        btn.longFired = true;
                        btn.lastRepeat = now;
                        switch (i) {
                            case 0: pushEvent(BTN_UP_LONG); break;
                            case 1: pushEvent(BTN_DOWN_LONG); break;
                            case 2: pushEvent(BTN_OK_LONG); break;
                        }
                    }
                } else if (i != 2) {
                    // Repeat while held (UP/DOWN only, not OK)
                    if ((now - btn.lastRepeat) >= BTN_REPEAT_RATE_MS) {
                        btn.lastRepeat = now;
                        pushEvent(i == 0 ? BTN_UP_REPEAT : BTN_DOWN_REPEAT);
                    }
                }
            }
            continue;
        }

        // State transition
        btn.stable = raw;

        if (raw == LOW) {
            // Press - reset long-press flag
            btn.longFired = false;
        } else {
            // Release - check short/double
            uint32_t holdDuration = now - btn.lastChange;

            if (!btn.longFired) {
                if ((now - btn.lastRelease) < BTN_DOUBLE_GAP_MS) {
                    switch (i) {
                        case 0: pushEvent(BTN_UP_DOUBLE); break;
                        case 1: pushEvent(BTN_DOWN_DOUBLE); break;
                        case 2: pushEvent(BTN_OK_DOUBLE); break;
                    }
                } else {
                    switch (i) {
                        case 0: pushEvent(BTN_UP_SHORT); break;
                        case 1: pushEvent(BTN_DOWN_SHORT); break;
                        case 2: pushEvent(BTN_OK_SHORT); break;
                    }
                }
            }
            btn.lastRelease = now;
        }
    }
}
