#include "../chinese_glyphs.h"
#include "sleep_timer.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createSleepTimer() { return new SleepTimer(); }

void SleepTimer::init() {
    sleepSeconds = 10; confirmStart = false;
    countdownStart = 0; stepMode = 0;
}

void SleepTimer::enter() {
    sleepSeconds = min((uint32_t)10, (uint32_t)4200); confirmStart = false;
    countdownStart = 0; stepMode = 0;
    displayMgr.setDirty();
}

void SleepTimer::exit() {}

void SleepTimer::formatDuration(char* buf, size_t sz) {
    if (sleepSeconds >= 3600) {
        snprintf(buf, sz, "%luh %lum %lus",
                 (unsigned long)(sleepSeconds / 3600),
                 (unsigned long)((sleepSeconds % 3600) / 60),
                 (unsigned long)(sleepSeconds % 60));
    } else if (sleepSeconds >= 60) {
        snprintf(buf, sz, "%lum %lus",
                 (unsigned long)(sleepSeconds / 60),
                 (unsigned long)(sleepSeconds % 60));
    } else {
        snprintf(buf, sz, "%lus", (unsigned long)sleepSeconds);
    }
}

void SleepTimer::update() {
    if (confirmStart && millis() - countdownStart > 3000) {
        // Start deep sleep after 3s countdown
        ESP.deepSleep(sleepSeconds * 1000000ULL);
    }
}

void SleepTimer::handleButton(ButtonEvent ev) {
    if (confirmStart) {
        // Let long/double OK through so core can navigateBack()
        if (ev == BTN_OK_LONG || ev == BTN_OK_DOUBLE) {
            return;
        }
        if (ev != BTN_NONE) {
            confirmStart = false;
            displayMgr.setDirty();
        }
        return;
    }

    switch (ev) {
        case BTN_UP_SHORT: {
            uint32_t step;
            switch (stepMode) {
                case 0: step = 1; break;
                case 1: step = 10; break;
                case 2: step = 60; break;
                case 3: step = 600; break;
                default: step = 1; break;
            }
            sleepSeconds += step;
            if (sleepSeconds > 4200) sleepSeconds = 4200;  // ESP8266 RTC max ~71 min
            displayMgr.setDirty();
            break;
        }
        case BTN_DOWN_SHORT: {
            uint32_t step;
            switch (stepMode) {
                case 0: step = 1; break;
                case 1: step = 10; break;
                case 2: step = 60; break;
                case 3: step = 600; break;
                default: step = 1; break;
            }
            if (sleepSeconds > step) sleepSeconds -= step;
            else sleepSeconds = 1;
            displayMgr.setDirty();
            break;
        }
        case BTN_OK_SHORT:
            confirmStart = true;
            countdownStart = millis();
            displayMgr.setDirty();
            break;
        case BTN_UP_DOUBLE:
            if (stepMode < 3) stepMode++;
            displayMgr.setDirty();
            break;
        case BTN_DOWN_DOUBLE:
            if (stepMode > 0) stepMode--;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void SleepTimer::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    if (confirmStart) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 10, 28, "即将休眠...");
        int remaining = 3 - (int)((millis() - countdownStart) / 1000);
        if (remaining < 0) remaining = 0;
        u8g2.setFont(FONT_BIG);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", remaining);
        uint8_t tw = u8g2.getStrWidth(buf);
        u8g2.drawStr((OLED_WIDTH - tw) / 2, 45, buf);
        u8g2.setFont(FONT_DATA);
        drawCN(u8g2, 0, 63, "任意键取消");
        return;
    }

    // Duration display
    u8g2.setFont(FONT_BIG);
    char buf[32];
    formatDuration(buf, sizeof(buf));
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 30, buf);

    // Step mode indicator
    u8g2.setFont(FONT_BODY);
    const char* stepNames[] = {"1s", "10s", "1m", "10m"};
    snprintf(buf, sizeof(buf), "Step: %s", stepNames[stepMode]);
    u8g2.drawStr(2, 42, buf);

    // Instructions
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(2, 55, "UP/DN adjust  OK=sleep");
    u8g2.setFont(FONT_DATA);
}
