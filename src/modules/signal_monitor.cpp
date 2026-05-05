#include "signal_monitor.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>

ModuleInterface* createSignalMonitor() { return new SignalMonitor(); }

void SignalMonitor::init() {
    memset(apSsid, 0, sizeof(apSsid));
    memset(apBssid, 0, sizeof(apBssid));
    apChannel = 0;
    currentRssi = -100;
    prevRssi = -100;
    memset(rssiHistory, -100, sizeof(rssiHistory));
    histIdx = 0;
    lastUpdate = 0;
    state = SIGMON_IDLE;
}

void SignalMonitor::enter() {
    memset(apSsid, 0, sizeof(apSsid));
    memset(apBssid, 0, sizeof(apBssid));
    apChannel = 0;
    currentRssi = -100;
    prevRssi = -100;
    memset(rssiHistory, -100, sizeof(rssiHistory));
    histIdx = 0;
    lastUpdate = 0;
    state = SIGMON_SCANNING;
    WiFi.scanNetworks(true, false);  // async scan
    displayMgr.setDirty();
}

void SignalMonitor::exit() {
    WiFi.scanDelete();
    state = SIGMON_IDLE;
}

void SignalMonitor::update() {
    if (state == SIGMON_SCANNING) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            if (n > 0) {
                int bestIdx = 0;
                int8_t bestRssi = -100;
                for (int i = 0; i < n; i++) {
                    if (WiFi.RSSI(i) > bestRssi) {
                        bestRssi = WiFi.RSSI(i);
                        bestIdx = i;
                    }
                }
                strCopySafe(apSsid, WiFi.SSID(bestIdx).c_str(), sizeof(apSsid));
                memcpy(apBssid, WiFi.BSSID(bestIdx), 6);
                apChannel = WiFi.channel(bestIdx);
                currentRssi = bestRssi;
                state = SIGMON_TRACKING;
            } else {
                state = SIGMON_IDLE;
            }
            lastUpdate = millis();
            displayMgr.setDirty();
        }
        return;
    }

    if (state != SIGMON_TRACKING) return;

    static bool waitingScan = false;
    static uint32_t scanStart = 0;

    if (!waitingScan && millis() - lastUpdate > 500) {
        WiFi.scanNetworks(true, false);
        waitingScan = true;
        scanStart = millis();
    }

    if (waitingScan) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            int8_t bestMatch = -100;
            for (int i = 0; i < n; i++) {
                if (memcmp(WiFi.BSSID(i), apBssid, 6) == 0) {
                    bestMatch = WiFi.RSSI(i);
                    break;
                }
            }
            if (bestMatch <= -100) {
                for (int i = 0; i < n; i++) {
                    if (strcmp(WiFi.SSID(i).c_str(), apSsid) == 0) {
                        if (WiFi.RSSI(i) > bestMatch) bestMatch = WiFi.RSSI(i);
                    }
                }
            }
            WiFi.scanDelete();
            waitingScan = false;

            if (bestMatch > -100) {
                prevRssi = currentRssi;
                currentRssi = bestMatch;
                rssiHistory[histIdx % MONITOR_HISTORY] = currentRssi;
                histIdx++;
                displayMgr.setDirty();
            }
            lastUpdate = millis();
        } else if (millis() - scanStart > WIFI_SCAN_TIMEOUT) {
            WiFi.scanDelete();
            waitingScan = false;
            lastUpdate = millis();
        }
    }
}

void SignalMonitor::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        state = SIGMON_SCANNING;
        WiFi.scanNetworks(true, false);
        lastUpdate = millis();
        displayMgr.setDirty();
    }
}

void SignalMonitor::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Signal Monitor");

    if (state == SIGMON_SCANNING) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(10, 35, "Scanning APs...");
        u8g2.drawStr(0, 63, "OK to rescan");
        return;
    }

    if (state == SIGMON_IDLE) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(5, 35, "No APs found");
        u8g2.drawStr(0, 63, "OK to rescan");
        return;
    }

    // AP SSID (truncated)
    u8g2.setFont(FONT_BODY);
    char ssidBuf[22];
    strCopySafe(ssidBuf, apSsid, sizeof(ssidBuf));
    u8g2.drawStr(2, 24, ssidBuf);

    // Big RSSI number
    u8g2.setFont(FONT_BIG);
    char buf[10];
    snprintf(buf, sizeof(buf), "%d", currentRssi);
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 42, buf);

    u8g2.setFont(FONT_SMALL);
    u8g2.drawStr((OLED_WIDTH - tw) / 2 + tw + 2, 42, "dBm");

    // Trend arrow
    u8g2.setFont(FONT_DATA);
    const char* arrow;
    if (currentRssi > prevRssi + 2) arrow = "^";
    else if (currentRssi < prevRssi - 2) arrow = "v";
    else arrow = "-";

    snprintf(buf, sizeof(buf), "Trend:%s CH:%d", arrow, (int)apChannel);
    u8g2.drawStr(2, 55, buf);

    // Footer
    u8g2.drawStr(0, 63, "OK to rescan");
}
