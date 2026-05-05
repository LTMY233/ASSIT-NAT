#include "rssi_drift.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"

ModuleInterface* createRssiDrift() { return new RssiDrift(); }

void RssiDrift::init() {
    memset(samples, 0, sizeof(samples));
    sampleIdx = 0; sampleCount = 0;
    currentRssi = -100; minRssi = 0; maxRssi = -100;
    lastSample = 0;
}

void RssiDrift::enter() {
    sampleIdx = 0; sampleCount = 0;
    currentRssi = -100; minRssi = 0; maxRssi = -100;
    lastSample = millis();
    displayMgr.setDirty();
}

void RssiDrift::exit() {}

void RssiDrift::update() {
    if (wifiMgr.isConnected() && millis() - lastSample > 1000) {
        currentRssi = wifiMgr.rssi();
        samples[sampleIdx] = currentRssi;
        sampleIdx = (sampleIdx + 1) % DRIFT_SAMPLES;
        if (sampleCount < DRIFT_SAMPLES) sampleCount++;
        if (currentRssi > maxRssi || maxRssi == -100) maxRssi = currentRssi;
        if (currentRssi < minRssi) minRssi = currentRssi;
        lastSample = millis();
        displayMgr.setDirty();
    }
}

void RssiDrift::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        sampleIdx = 0; sampleCount = 0;
        minRssi = 0; maxRssi = -100;
        displayMgr.setDirty();
    }
}

void RssiDrift::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "RSSI Drift Log");

    if (!wifiMgr.isConnected()) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(10, 35, "Connect Wi-Fi first");
        return;
    }

    u8g2.setFont(FONT_DATA);
    char buf[30];
    snprintf(buf, sizeof(buf), "Now:%d min:%d max:%d", currentRssi, minRssi, maxRssi);
    u8g2.drawStr(0, 24, buf);

    // Line chart (-30 to -90 dBm)
    uint8_t baseY = 50;
    for (uint8_t i = 0; i < sampleCount - 1 && i < 127; i++) {
        uint8_t idx1 = (sampleIdx + DRIFT_SAMPLES - sampleCount + i) % DRIFT_SAMPLES;
        uint8_t idx2 = (sampleIdx + DRIFT_SAMPLES - sampleCount + i + 1) % DRIFT_SAMPLES;
        uint8_t y1 = mapRange(samples[idx1], -90, -30, baseY, baseY - 25);
        uint8_t y2 = mapRange(samples[idx2], -90, -30, baseY, baseY - 25);
        u8g2.drawLine(i, y1, i + 1, y2);
    }

    u8g2.drawStr(0, 63, "OK to reset");
}
