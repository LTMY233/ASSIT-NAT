#pragma once
#include <Arduino.h>
#include "modules/module_interface.h"

class CoreSystem {
public:
    void init();
    void tick();

    SystemState getState() const { return lastState; }

private:
    SystemState lastState;
    uint32_t    heapCheckTimer;

    void processButtons();
    void checkHeap();
    void handleStateTransitions();
};

extern CoreSystem coreSystem;
