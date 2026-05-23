#include "../chinese_glyphs.h"
#include "rf_noise.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>

ModuleInterface* createRfNoise() { return new RfNoise(); }

void RfNoise::init() {
    currentCh = 1;
    noiseFloor = -95;
    memset(chNoise, -95, sizeof(chNoise));
    lastSample = 0;
    samplesCollected = 0;
    measuring = false;
}

void RfNoise::enter() {
    currentCh = 1;
    noiseFloor = -95;
    memset(chNoise, -95, sizeof(chNoise));
    lastSample = millis();
    samplesCollected = 0;
    measuring = true;
    wifi_set_channel(currentCh);
    displayMgr.setDirty();
}

void RfNoise::exit() {}

int8_t RfNoise::getMinRssiOnChannel() {
    // Scan for APs and return the weakest RSSI (approx noise floor)
    int8_t minRssi = -30;
    int n = WiFi.scanNetworks(false, true, 0);
    if (n < 0) return -95;

    bool found = false;
    for (int i = 0; i < n; i++) {
        int8_t r = WiFi.RSSI(i);
        if (r < minRssi) {
            minRssi = r;
            found = true;
        }
    }
    WiFi.scanDelete();
    // Noise floor is typically below the weakest AP
    if (found) return minRssi - 5;
    return -90;
}

void RfNoise::measureNoise() {
    if (!measuring) return;
    if (millis() - lastSample < 800) return;

    noiseFloor = getMinRssiOnChannel();
    chNoise[currentCh] = noiseFloor;

    currentCh++;
    if (currentCh > 13) {
        currentCh = 1;
        samplesCollected++;
    }
    wifi_set_channel(currentCh);
    lastSample = millis();
    displayMgr.setDirty();
}

void RfNoise::update() {
    measureNoise();
}

void RfNoise::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        measuring = !measuring;
        displayMgr.setDirty();
    } else if (ev == BTN_UP_SHORT) {
        currentCh = 1;
        memset(chNoise, -95, sizeof(chNoise));
        samplesCollected = 0;
        lastSample = 0;
        wifi_set_channel(currentCh);
        displayMgr.setDirty();
    }
}

void RfNoise::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // Current channel and noise level
    u8g2.setFont(FONT_BIG);
    char buf[16];
    snprintf(buf, sizeof(buf), "CH%d", currentCh);
    u8g2.drawStr(10, 30, buf);

    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "噪声:%d dBm", noiseFloor);
    drawCN(u8g2, 2, 42, buf);

    // Noise bar
    uint8_t barW = mapRange(noiseFloor, -95, -30, 0, 100);
    if (barW > 100) barW = 100;
    u8g2.drawFrame(14, 48, 100, 5);
    if (barW > 0) {
        u8g2.drawBox(14, 48, barW, 5);
    }

    // Pass count
    snprintf(buf, sizeof(buf), "扫描:%d %s",
             samplesCollected, measuring ? "运行" : "停止");
    drawCN(u8g2, 0, 57, buf);

    // Footer
}
