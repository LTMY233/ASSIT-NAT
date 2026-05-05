#include "logic_probe.h"
#include "../core/display_mgr.h"
#include "../config.h"

ModuleInterface* createLogicProbe() { return new LogicProbe(); }

void LogicProbe::init() {
    pin = PIN_GPIO_PROBE; level = false;
    lastRead = 0; highCount = 0; lowCount = 0; transitions = 0;
}

void LogicProbe::enter() {
    pinMode(pin, INPUT_PULLUP);
    level = digitalRead(pin);
    highCount = 0; lowCount = 0; transitions = 0;
    lastRead = millis();
    displayMgr.setDirty();
}

void LogicProbe::exit() {
    pinMode(pin, INPUT);  pinMode(pin, INPUT);  // restore default
}

void LogicProbe::update() {
    if (millis() - lastRead > 200) {
        bool newLevel = digitalRead(pin);
        if (newLevel != level) {
            transitions++;
            level = newLevel;
        }
        if (level) highCount++; else lowCount++;
        lastRead = millis();
        displayMgr.setDirty();
    }
}

void LogicProbe::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        highCount = 0; lowCount = 0; transitions = 0;
        displayMgr.setDirty();
    }
}

void LogicProbe::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Logic Probe");

    // Level indicator (large)
    u8g2.setFont(FONT_BIG);
    const char* levelStr = level ? "HIGH" : "LOW";
    uint8_t tw = u8g2.getStrWidth(levelStr);
    if (level) u8g2.drawBox((OLED_WIDTH - tw) / 2 - 4, 12, tw + 8, 22);
    u8g2.setDrawColor(level ? 0 : 1);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 30, levelStr);
    u8g2.setDrawColor(1);

    u8g2.setFont(FONT_DATA);
    char buf[30];
    snprintf(buf, sizeof(buf), "Pin:GPIO%d", pin);
    u8g2.drawStr(2, 44, buf);
    snprintf(buf, sizeof(buf), "H:%lu L:%lu T:%lu", highCount, lowCount, transitions);
    u8g2.drawStr(2, 63, buf);
}
