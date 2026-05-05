#include "wifi_scanner.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>

ScanRecord WifiScanner::results[WIFI_SCAN_MAX_AP];
uint8_t WifiScanner::resultCount = 0;

ModuleInterface* createWifiScanner() { return new WifiScanner(); }

void WifiScanner::init() {
    state = IDLE;
    cursor = 0;
    scrollOffset = 0;
    scanStart = 0;
    resultCount = 0;
}

void WifiScanner::enter() {
    cursor = 0;
    scrollOffset = 0;
    state = IDLE;
    startScan();
    displayMgr.setDirty();
}

void WifiScanner::exit() {
}

void WifiScanner::startScan() {
    state = SCANNING;
    scanStart = millis();
    WiFi.scanNetworks(true, false);
    resultCount = 0;
}

void WifiScanner::update() {
    if (state == SCANNING) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            resultCount = (n > WIFI_SCAN_MAX_AP) ? WIFI_SCAN_MAX_AP : n;
            for (uint8_t i = 0; i < resultCount; i++) {
                strCopySafe(results[i].ssid, WiFi.SSID(i).c_str(), sizeof(results[i].ssid));
                uint8_t* bssidPtr = WiFi.BSSID(i);
                memcpy(results[i].bssid, bssidPtr, 6);
                results[i].channel = WiFi.channel(i);
                results[i].rssi = WiFi.RSSI(i);
                results[i].encType = WiFi.encryptionType(i);
            }
            sortByRssi();
            state = RESULTS;
            displayMgr.setDirty();
        } else if (millis() - scanStart > WIFI_SCAN_TIMEOUT) {
            WiFi.scanDelete();
            state = RESULTS;
            displayMgr.setDirty();
        }
    }
}

void WifiScanner::sortByRssi() {
    for (uint8_t i = 0; i < resultCount; i++) {
        for (uint8_t j = i + 1; j < resultCount; j++) {
            if (results[j].rssi > results[i].rssi) {
                ScanRecord tmp = results[i];
                results[i] = results[j];
                results[j] = tmp;
            }
        }
    }
}

const char* WifiScanner::encTypeStr(uint8_t enc) {
    switch (enc) {
        case ENC_TYPE_NONE: return "O";
        case ENC_TYPE_WEP:  return "W";
        case ENC_TYPE_TKIP: return "A";
        case ENC_TYPE_CCMP: return "2";
        case ENC_TYPE_AUTO: return "3";
        default: return "?";
    }
}

const ScanRecord* WifiScanner::getResults(uint8_t& count) {
    count = resultCount;
    return results;
}

void WifiScanner::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_UP_SHORT:
            if (cursor > 0) { cursor--; displayMgr.setDirty(); }
            break;
        case BTN_DOWN_SHORT:
            if (cursor < resultCount - 1) { cursor++; displayMgr.setDirty(); }
            break;
        case BTN_OK_SHORT:
            startScan();
            displayMgr.setDirty();
            break;
        default: break;
    }

    // Scroll follow
    uint8_t vis = 2;
    if (cursor < scrollOffset) scrollOffset = cursor;
    if (cursor >= scrollOffset + vis) scrollOffset = cursor - vis + 1;
}

void WifiScanner::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 8, "WiFi Scanner");

    if (state == SCANNING) {
        u8g2.setFont(FONT_MENU);
        u8g2.drawStr(20, 40, "Scanning...");
    } else if (resultCount == 0) {
        u8g2.setFont(FONT_MENU);
        u8g2.drawStr(10, 40, "No networks");
    } else {
        // Header
        u8g2.setFont(FONT_DATA);
        u8g2.drawHLine(0, 13, OLED_WIDTH);
        u8g2.drawStr(0,   23, "SSID");
        u8g2.drawStr(78,  23, "CH");
        u8g2.drawStr(98,  23, "dBm");
        u8g2.drawStr(115, 23, "En");

        uint8_t vis = 2;
        for (uint8_t i = 0; i < vis && (scrollOffset + i) < resultCount; i++) {
            uint8_t idx = scrollOffset + i;
            uint8_t y = 38 + i * 15;

            if (idx == cursor) {
                u8g2.drawBox(0, y - 11, OLED_WIDTH, 14);
                u8g2.setDrawColor(0);
            }

            // SSID truncation
            char buf[13];
            strCopySafe(buf, results[idx].ssid, 12);
            u8g2.drawStr(0, y, buf);

            char tmp[8];
            itoaSimple(results[idx].channel, tmp, 10);
            u8g2.drawStr(78, y, tmp);

            itoaSimple(results[idx].rssi, tmp, 10);
            u8g2.drawStr(98, y, tmp);

            u8g2.drawStr(115, y, encTypeStr(results[idx].encType));

            if (idx == cursor) {
                u8g2.setDrawColor(1);
            }
        }

        // Bottom bar
        u8g2.setFont(FONT_DATA);
        char foot[32];
        snprintf(foot, sizeof(foot), "%d/%d nets  OK:rescan",
                 cursor + 1, resultCount);
        u8g2.drawStr(0, 63, foot);
    }
}
