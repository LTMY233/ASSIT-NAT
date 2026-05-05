#include "channel_heatmap.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>

ModuleInterface* createChannelHeatmap() { return new ChannelHeatmap(); }

void ChannelHeatmap::init() {
    memset(chCounts, 0, sizeof(chCounts));
    maxCount = 0;
    lastScan = 0;
    autoRefresh = true;
    scanning = false;
}

void ChannelHeatmap::enter() {
    memset(chCounts, 0, sizeof(chCounts));
    maxCount = 0;
    lastScan = millis();
    scanning = false;
    doScan();
    displayMgr.setDirty();
}

void ChannelHeatmap::exit() {
    WiFi.scanDelete();
}

void ChannelHeatmap::doScan() {
    if (scanning) return;
    scanning = true;
    WiFi.scanNetworks(true, false);
}

void ChannelHeatmap::update() {
    if (scanning) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            memset(chCounts, 0, sizeof(chCounts));
            maxCount = 0;
            for (int i = 0; i < n; i++) {
                uint8_t ch = WiFi.channel(i);
                if (ch >= 1 && ch <= 13) {
                    chCounts[ch]++;
                    if (chCounts[ch] > maxCount) maxCount = chCounts[ch];
                }
            }
            WiFi.scanDelete();
            scanning = false;
            displayMgr.setDirty();
        } else if (millis() - lastScan > WIFI_SCAN_TIMEOUT) {
            WiFi.scanDelete();
            scanning = false;
        }
    } else if (autoRefresh && millis() - lastScan > 5000) {
        lastScan = millis();
        doScan();
    }
}

void ChannelHeatmap::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        lastScan = millis();
        doScan();
        displayMgr.setDirty();
    } else if (ev == BTN_UP_SHORT) {
        autoRefresh = !autoRefresh;
        displayMgr.setDirty();
    }
}

void ChannelHeatmap::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Channel Heatmap");

    if (maxCount == 0) {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(10, 35, scanning ? "Scanning..." : "No data");
        return;
    }

    // Bar chart area: x=8~120, y=15~50
    uint8_t barW = 7;
    uint8_t gap = 2;
    uint8_t baseY = 50;
    uint8_t maxH = 35;

    u8g2.setFont(FONT_SMALL);

    for (uint8_t ch = 1; ch <= 13; ch++) {
        uint8_t x = 8 + (ch - 1) * (barW + gap);
        uint8_t h = (maxCount > 0) ? (chCounts[ch] * maxH / maxCount) : 0;

        if (h > 0) {
            // Fill by occupancy: low=empty, high=solid
            if (chCounts[ch] >= 5) {
                u8g2.drawBox(x, baseY - h, barW, h);
            } else if (chCounts[ch] >= 3) {
                for (uint8_t py = baseY - h; py < baseY; py += 2) {
                    u8g2.drawHLine(x, py, barW);
                }
            } else {
                u8g2.drawFrame(x, baseY - h, barW, h);
            }

            // AP count label
            char buf[3];
            itoaSimple(chCounts[ch], buf, 10);
            u8g2.drawStr(x + 1, baseY - h - 2, buf);
        }

        // Channel number
        char chBuf[4];
        itoaSimple(ch, chBuf, 10);
        u8g2.drawStr(x, baseY + 8, chBuf);
    }

    // Auto-refresh status
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 63, autoRefresh ? "Auto:ON  UP:toggle" : "Auto:OFF  UP:toggle");
}
