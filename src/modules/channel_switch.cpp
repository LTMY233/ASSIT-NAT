#include "../chinese_glyphs.h"
#include "channel_switch.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createChannelSwitch() { return new ChannelSwitch(); }

void ChannelSwitch::init() {
    fromChannel = 1; toChannel = 6;
    switchTime = 0; measured = false;
    phase = 0; startUs = 0;
}

void ChannelSwitch::enter() {
    measured = false; phase = 0;
    displayMgr.setDirty();
}

void ChannelSwitch::exit() {}

void ChannelSwitch::update() {
    if (phase == 1) {
        // Wait for WiFi channel switch
        delayMicroseconds(100);
        uint32_t elapsed = micros() - startUs;
        if (elapsed > 50000) {  // max wait 50ms
            switchTime = elapsed;
            measured = true;
            phase = 2;
            displayMgr.setDirty();
        }
    }
}

void ChannelSwitch::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT && phase != 1) {
        // Start measurement
        wifi_set_channel(fromChannel);
        delay(10);
        startUs = micros();
        wifi_set_channel(toChannel);
        switchTime = micros() - startUs;
        measured = true;
        phase = 2;
        displayMgr.setDirty();
    }
    if (ev == BTN_UP_SHORT) {
        fromChannel = (fromChannel % 13) + 1;
        measured = false; displayMgr.setDirty();
    }
    if (ev == BTN_DOWN_SHORT) {
        toChannel = (toChannel % 13) + 1;
        measured = false; displayMgr.setDirty();
    }
}

void ChannelSwitch::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    u8g2.setFont(FONT_BODY);
    char buf[30];
    snprintf(buf, sizeof(buf), "CH %d -> CH %d", fromChannel, toChannel);
    u8g2.drawStr(2, 25, buf);

    if (measured) {
        u8g2.setFont(FONT_BIG);
        snprintf(buf, sizeof(buf), "%luus", switchTime);
        uint8_t tw = u8g2.getStrWidth(buf);
        u8g2.drawStr((OLED_WIDTH - tw) / 2, 40, buf);
    }

    u8g2.setFont(FONT_DATA);
}
