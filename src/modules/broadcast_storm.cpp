#include "broadcast_storm.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"

BroadcastStorm* BroadcastStorm::instance = nullptr;

ModuleInterface* createBroadcastStorm() { return new BroadcastStorm(); }

void BroadcastStorm::init() {
    totalPkts = 0; broadcastPkts = 0;
    broadcastPercent = 0; alert = false;
    lastCalc = 0; running = false;
    instance = this;
}

void BroadcastStorm::enter() {
    totalPkts = 0; broadcastPkts = 0;
    broadcastPercent = 0; alert = false;
    lastCalc = millis(); instance = this;
    wifiMgr.promiscuousAcquire(getId());
    running = true;
    displayMgr.setDirty();
}

void BroadcastStorm::exit() {
    running = false;
    wifiMgr.promiscuousRelease(getId());
    instance = nullptr;
}

void BroadcastStorm::onPacket(uint8_t* buf, uint16_t len) {
    if (!instance || len < 24) return;

    instance->totalPkts++;
    // Check if dest is broadcast (FF:FF:FF:FF:FF:FF)
    bool isBcast = true;
    for (int i = 4; i < 10; i++) {
        if (buf[i] != 0xFF) { isBcast = false; break; }
    }
    if (isBcast) instance->broadcastPkts++;
}

void BroadcastStorm::update() {
    if (!running) return;

    if (millis() - lastCalc > 1000) {
        if (totalPkts > 0) {
            broadcastPercent = (uint8_t)(broadcastPkts * 100 / totalPkts);
        }
        alert = (broadcastPercent > 50);  // alert above 50%
        totalPkts = 0; broadcastPkts = 0;
        lastCalc = millis();
        displayMgr.setDirty();
    }
}

void BroadcastStorm::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        totalPkts = 0; broadcastPkts = 0; alert = false;
        displayMgr.setDirty();
    }
}

void BroadcastStorm::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Broadcast Ratio Monitor");

    if (alert) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(2, 24, "⚠ Broadcast storm alert!");
    }

    u8g2.setFont(FONT_BIG);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", broadcastPercent);
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 35, buf);

    // Threshold bar
    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "Thresh:50%%");
    u8g2.drawStr(2, 48, buf);

    uint8_t barW = broadcastPercent * OLED_WIDTH / 100;
    u8g2.drawFrame(0, 52, OLED_WIDTH, 4);
    u8g2.drawBox(0, 52, barW, 4);

    u8g2.drawStr(0, 63, alert ? "Storm detected!" : "Normal");
}
