#pragma once
#include <Arduino.h>
#include "modules/module_interface.h"

class ButtonMgr {
public:
    void init(uint8_t pinUp, uint8_t pinDown, uint8_t pinOk);
    void update();  // Call each loop, sample & generate events

    // Pop next button event (FIFO)
    bool getEvent(ButtonEvent& ev);
    // Check pending events
    bool hasEvents() const;
    // Flush event queue
    void flush();

private:
    struct BtnState {
        uint8_t  pin;
        bool     lastRaw;
        bool     stable;
        uint32_t lastChange;
        uint32_t lastRelease;
        uint32_t lastRepeat;
        bool     longFired;
    };

    static const uint8_t QUEUE_SIZE = 8;
    ButtonEvent eventQueue[QUEUE_SIZE];
    uint8_t     queueHead, queueTail;

    BtnState btns[3];

    void pushEvent(ButtonEvent ev);
};

extern ButtonMgr buttonMgr;
