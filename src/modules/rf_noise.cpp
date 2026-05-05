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
    u8g2.drawStr(0, 9, "RF Noise Floor");

    // Current channel and noise level
    u8g2.setFont(FONT_BIG);
    char buf[16];
    snprintf(buf, sizeof(buf), "CH%d", currentCh);
    u8g2.drawStr(10, 30, buf);

    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "Noise:%d dBm", noiseFloor);
    u8g2.drawStr(2, 42, buf);

    // Noise bar
    uint8_t barW = mapRange(noiseFloor, -95, -30, 0, 100);
    if (barW > 100) barW = 100;
    u8g2.drawFrame(14, 48, 100, 5);
    if (barW > 0) {
        u8g2.drawBox(14, 48, barW, 5);
    }

    // Pass count
    snprintf(buf, sizeof(buf), "Passes:%d %s",
             samplesCollected, measuring ? "RUN" : "STOP");
    u8g2.drawStr(0, 57, buf);

    // Footer
    u8g2.drawStr(0, 63, measuring ? "OK:stop UP:reset" : "OK:start UP:reset");
}
