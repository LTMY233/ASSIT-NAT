#include "rssi_histogram.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include <ESP8266WiFi.h>
#include "../utils.h"

ModuleInterface* createRssiHistogram() { return new RssiHistogram(); }

void RssiHistogram::init() {
    memset(bins, 0, sizeof(bins));
    maxBin = 0; hasData = false; lastScan = 0;
}

void RssiHistogram::enter() {
    memset(bins, 0, sizeof(bins));
    maxBin = 0; hasData = false; lastScan = 0;
    doScan();
    displayMgr.setDirty();
}

void RssiHistogram::exit() { WiFi.scanDelete(); }

void RssiHistogram::doScan() {
    WiFi.scanNetworks(true, false);
    lastScan = millis();
}

static uint8_t rssiToBin(int8_t rssi) {
    if (rssi >= -30) return 0;
    if (rssi >= -40) return 1;
    if (rssi >= -50) return 2;
    if (rssi >= -60) return 3;
    if (rssi >= -70) return 4;
    if (rssi >= -80) return 5;
    return 6;
}

void RssiHistogram::update() {
    int n = WiFi.scanComplete();
    if (n >= 0) {
        memset(bins, 0, sizeof(bins));
        for (int i = 0; i < n; i++) {
            uint8_t bin = rssiToBin(WiFi.RSSI(i));
            bins[bin]++;
        }
        WiFi.scanDelete();
        maxBin = 0;
        for (uint8_t i = 0; i < 7; i++) {
            if (bins[i] > maxBin) maxBin = bins[i];
        }
        hasData = true;
        displayMgr.setDirty();
    } else if (n < 0 && millis() - lastScan > WIFI_SCAN_TIMEOUT) {
        WiFi.scanDelete();
    }
}

void RssiHistogram::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) { hasData = false; doScan(); displayMgr.setDirty(); }
}

void RssiHistogram::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "RSSI Histogram");

    if (!hasData) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(10, 35, "Scanning...");
        return;
    }

    static const char* labels[] = {">-30", "-40", "-50", "-60", "-70", "-80", "<-90"};
    uint8_t barW = 14;
    uint8_t baseY = 48;
    uint8_t maxH = 24;

    u8g2.setFont(FONT_DATA);
    for (uint8_t i = 0; i < 7; i++) {
        uint8_t x = 4 + i * (barW + 3);
        uint8_t h = (maxBin > 0) ? (bins[i] * maxH / maxBin) : 0;

        if (h > 0) u8g2.drawBox(x, baseY - h, barW, h);

        // Label
        u8g2.drawStr(x - 1, baseY + 8, labels[i]);

        // Count
        if (bins[i] > 0) {
            char buf[4]; itoaSimple(bins[i], buf, 10);
            uint8_t tw = u8g2.getStrWidth(buf);
            u8g2.drawStr(x + (barW - tw) / 2, baseY - h - 8, buf);
        }
    }

    u8g2.drawStr(0, 63, "OK to rescan");
}
