#include "../chinese_glyphs.h"
#include "button_test.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createButtonTest() { return new ButtonTest(); }

void ButtonTest::init() {
    upPressed = false; downPressed = false; okPressed = false;
    upPressStart = 0; downPressStart = 0; okPressStart = 0;
    upDuration = 0; downDuration = 0; okDuration = 0;
    lastEventTime = 0; lastEventName[0] = '\0';
}

void ButtonTest::enter() {
    upPressed = false; downPressed = false; okPressed = false;
    upDuration = 0; downDuration = 0; okDuration = 0;
    lastEventTime = 0; lastEventName[0] = '\0';
    displayMgr.setDirty();
}

void ButtonTest::exit() {}

void ButtonTest::update() {
    uint32_t now = millis();
    // Track press durations
    if (upPressed)   upDuration   = now - upPressStart;
    if (downPressed) downDuration = now - downPressStart;
    if (okPressed)   okDuration   = now - okPressStart;
}

void ButtonTest::handleButton(ButtonEvent ev) {
    lastEventTime = millis();
    switch (ev) {
        case BTN_UP_SHORT:
            upPressed = true; upPressStart = lastEventTime;
            strcpy(lastEventName, "UP SHORT");
            break;
        case BTN_DOWN_SHORT:
            downPressed = true; downPressStart = lastEventTime;
            strcpy(lastEventName, "DOWN SHORT");
            break;
        case BTN_OK_SHORT:
            okPressed = true; okPressStart = lastEventTime;
            strcpy(lastEventName, "OK SHORT");
            break;
        case BTN_UP_LONG:
            upPressed = false; upDuration = 0;
            strcpy(lastEventName, "UP LONG");
            break;
        case BTN_DOWN_LONG:
            downPressed = false; downDuration = 0;
            strcpy(lastEventName, "DOWN LONG");
            break;
        case BTN_OK_LONG:
            okPressed = false; okDuration = 0;
            strcpy(lastEventName, "OK LONG");
            break;
        case BTN_UP_DOUBLE:
            upPressed = false; upDuration = 0;
            strcpy(lastEventName, "UP DOUBLE");
            break;
        case BTN_DOWN_DOUBLE:
            downPressed = false; downDuration = 0;
            strcpy(lastEventName, "DOWN DOUBLE");
            break;
        case BTN_OK_DOUBLE:
            okPressed = false; okDuration = 0;
            strcpy(lastEventName, "OK DOUBLE");
            break;
        default: break;
    }
    displayMgr.setDirty();
}

void ButtonTest::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    u8g2.setFont(FONT_BODY);
    char buf[40];
    uint8_t y = 24;

    // UP button state
    snprintf(buf, sizeof(buf), "UP: %s %lums",
             upPressed ? "HOLD" : "RELEASE",
             (unsigned long)upDuration);
    u8g2.drawStr(2, y, buf); y += 10;

    // DOWN button state
    snprintf(buf, sizeof(buf), "DN: %s %lums",
             downPressed ? "HOLD" : "RELEASE",
             (unsigned long)downDuration);
    u8g2.drawStr(2, y, buf); y += 10;

    // OK button state
    snprintf(buf, sizeof(buf), "OK: %s %lums",
             okPressed ? "HOLD" : "RELEASE",
             (unsigned long)okDuration);
    u8g2.drawStr(2, y, buf); y += 10;

    // Last event
    u8g2.setFont(FONT_DATA);
    if (lastEventName[0]) {
        snprintf(buf, sizeof(buf), "Last: %s", lastEventName);
        u8g2.drawStr(0, 63, buf);
    } else {
        drawCN(u8g2, 0, 63, "按任意键");
    }
}
