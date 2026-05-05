#include "spectrum_view.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>

ModuleInterface* createSpectrumView() { return new SpectrumView(); }

void SpectrumView::init() {
    memset(chRssi, 0, sizeof(chRssi));
    maxRssi = 0;
    lastScan = 0;
    scanning = false;
    autoRefresh = true;
}

void SpectrumView::enter() {
    memset(chRssi, 0, sizeof(chRssi));
    maxRssi = 0;
    lastScan = millis();
    scanning = false;
    autoRefresh = true;
    doScan();
    displayMgr.setDirty();
}

void SpectrumView::exit() {
    WiFi.scanDelete();
}

void SpectrumView::doScan() {
    if (scanning) return;
    scanning = true;
    WiFi.scanNetworks(true, false);
}

void SpectrumView::update() {
    if (scanning) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            // Find RSSI per channel by taking the strongest AP on each channel
            memset(chRssi, -90, sizeof(chRssi));
            for (int i = 0; i < n; i++) {
                uint8_t ch = WiFi.channel(i);
                int8_t r = WiFi.RSSI(i);
                if (ch >= 1 && ch <= 13) {
                    if (r > chRssi[ch]) chRssi[ch] = r;
                }
            }
            // Clamp unused channels
            for (uint8_t ch = 1; ch <= 13; ch++) {
                if (chRssi[ch] < -95) chRssi[ch] = -95;
            }
            maxRssi = -95;
            for (uint8_t ch = 1; ch <= 13; ch++) {
                if (chRssi[ch] > maxRssi) maxRssi = chRssi[ch];
            }
            WiFi.scanDelete();
            scanning = false;
            lastScan = millis();
            displayMgr.setDirty();
        } else if (millis() - lastScan > WIFI_SCAN_TIMEOUT) {
            WiFi.scanDelete();
            scanning = false;
        }
    } else if (autoRefresh && millis() - lastScan > SPECTRUM_SCAN_INTERVAL) {
        doScan();
    }
}

void SpectrumView::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        doScan();
        displayMgr.setDirty();
    } else if (ev == BTN_UP_SHORT) {
        autoRefresh = !autoRefresh;
        displayMgr.setDirty();
    }
}

void SpectrumView::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Spectrum View");

    if (maxRssi <= -95 && !scanning) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(10, 35, "Press OK to scan");
    } else if (scanning) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(20, 35, "Scanning...");
    } else {
        // Bar chart: CH1-13, x=5..122, y=16..52
        uint8_t barW = 7;
        uint8_t gap = 2;
        uint8_t baseY = 52;
        uint8_t maxH = 36;

        u8g2.setFont(FONT_SMALL);
        for (uint8_t ch = 1; ch <= 13; ch++) {
            uint8_t x = 2 + (ch - 1) * (barW + gap);
            // Map RSSI -95..-30 to height 0..maxH
            int8_t r = chRssi[ch];
            uint8_t h = mapRange(r, -95, -30, 0, maxH);

            if (h > 0) {
                if (r >= -50) {
                    // Strong: solid box
                    u8g2.drawBox(x, baseY - h, barW, h);
                } else if (r >= -70) {
                    // Medium: hash pattern
                    for (uint8_t py = baseY - h; py < baseY; py += 2) {
                        u8g2.drawHLine(x, py, barW);
                    }
                } else {
                    // Weak: outline
                    u8g2.drawFrame(x, baseY - h, barW, h);
                }
            }

            // Channel number
            char buf[4];
            itoaSimple(ch, buf, 10);
            u8g2.drawStr(x - 1, baseY + 7, buf);
        }
    }

    // Footer
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 63, autoRefresh ? "Auto:ON UP:off" : "Auto:OFF UP:on");
}
