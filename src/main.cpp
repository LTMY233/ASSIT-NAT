// ============================================================
// ESP8266 Swiss Army Knife
// Entry: setup() + loop()
// ============================================================

#include <Arduino.h>
#include "core/core_system.h"

void setup() {
    coreSystem.init();
}

void loop() {
    coreSystem.tick();
}
