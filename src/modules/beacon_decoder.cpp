#include "../chinese_glyphs.h"
#include "beacon_decoder.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"

BeaconDecoder* BeaconDecoder::instance = nullptr;

ModuleInterface* createBeaconDecoder() { return new BeaconDecoder(); }

void BeaconDecoder::init() {
    memset(&beacon, 0, sizeof(beacon));
    hasData = false; cursor = 0; running = false;
    instance = this;
}

void BeaconDecoder::enter() {
    hasData = false; cursor = 0; instance = this;
    wifiMgr.promiscuousAcquire(getId());
    running = true;
    displayMgr.setDirty();
}

void BeaconDecoder::exit() {
    running = false;
    wifiMgr.promiscuousRelease(getId());
    instance = nullptr;
}

bool BeaconDecoder::parseBeacon(uint8_t* buf, uint16_t len) {
    if (len < 36) return false;

    uint8_t fc = buf[0];
    if ((fc & 0x0C) != 0x00) return false;  // Management frame
    if (((fc >> 4) & 0x0F) != 0x08) return false;  // Beacon

    // Extract BSSID (addr3)
    memcpy(beacon.bssid, buf + 16, 6);
    // RSSI
    beacon.rssi = -(256 - buf[len - 1]);

    // Skip 802.11 header + fixed params (timestamp 8, interval 2, capability 2 = 12 bytes after addr3)
    uint8_t* body = buf + 36;
    uint16_t bodyLen = len - 36 - 4;  // subtract FCS

    beacon.rateCount = 0;
    beacon.htCapable = false;
    beacon.wpsEnabled = false;
    beacon.vendorLen = 0;
    beacon.country[0] = '\0';

    for (uint16_t i = 0; i < bodyLen && i < 200; ) {
        uint8_t elemId = body[i];
        uint8_t elemLen = body[i + 1];
        if (elemId == 0 && elemLen <= 32) {  // SSID
            memcpy(beacon.ssid, body + i + 2, elemLen);
            beacon.ssid[elemLen] = '\0';
        } else if (elemId == 1 && elemLen > 0) {  // Supported Rates
            for (uint8_t j = 0; j < elemLen && beacon.rateCount < 8; j++) {
                beacon.supportedRates[beacon.rateCount++] = body[i + 2 + j] & 0x7F;
            }
        } else if (elemId == 7 && elemLen >= 3) {  // Country
            memcpy(beacon.country, body + i + 2, 3);
            beacon.country[3] = '\0';
        } else if (elemId == 45) {  // HT Capabilities
            beacon.htCapable = true;
        } else if (elemId == 221 && elemLen >= 4) {  // Vendor Specific
            uint8_t oui[3]; memcpy(oui, body + i + 2, 3);
            if (oui[0] == 0x00 && oui[1] == 0x50 && oui[2] == 0xF2) {
                if (body[i + 5] == 0x04) {  // WPS element
                    beacon.wpsEnabled = true;
                }
            }
            if (elemLen < 32) {
                memcpy(beacon.vendorSpecific, body + i + 2, elemLen);
                beacon.vendorLen = elemLen;
            }
        }
        i += 2 + elemLen;
    }
    return true;
}

void BeaconDecoder::onPacket(uint8_t* buf, uint16_t len) {
    if (!instance || instance->hasData) return;  // Already collected, don't overwrite
    if (instance->parseBeacon(buf, len)) {
        instance->hasData = true;
    }
}

void BeaconDecoder::update() {
    if (!running) return;
    if (hasData) displayMgr.setDirty();
}

void BeaconDecoder::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT && cursor > 0) { cursor--; displayMgr.setDirty(); }
    if (ev == BTN_DOWN_SHORT && cursor < 8) { cursor++; displayMgr.setDirty(); }
    if (ev == BTN_OK_SHORT) { hasData = false; displayMgr.setDirty(); }  // Recapture
}

void BeaconDecoder::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    if (!hasData) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 10, 35, "等待中...");
        return;
    }

    u8g2.setFont(FONT_DATA);
    char buf[40];

    // Page display based on cursor
    switch (cursor) {
        case 0: snprintf(buf, sizeof(buf), "SSID:%s", beacon.ssid); break;
        case 1: {
            char macBuf[18]; macToStr(beacon.bssid, macBuf, sizeof(macBuf));
            snprintf(buf, sizeof(buf), "BSSID:%s", macBuf);
            break;
        }
        case 2: snprintf(buf, sizeof(buf), "RSSI:%d dBm", beacon.rssi); break;
        case 3: snprintf(buf, sizeof(buf), "Country:%s", beacon.country[0] ? beacon.country : "N/A"); break;
        case 4: {
            char rates[32] = "";
            for (uint8_t i = 0; i < beacon.rateCount; i++) {
                char r[5]; snprintf(r, sizeof(r), "%d ", beacon.supportedRates[i] * 5 / 10);
                strcat(rates, r);
            }
            snprintf(buf, sizeof(buf), "Rate:%sMbps", rates);
            break;
        }
        case 5: snprintf(buf, sizeof(buf), "HT:%s", beacon.htCapable ? "Yes" : "No"); break;
        case 6: snprintf(buf, sizeof(buf), "WPS:%s", beacon.wpsEnabled ? "On" : "Off"); break;
        case 7: {
            char hex[40]; hexDump(beacon.vendorSpecific, min((uint8_t)16, beacon.vendorLen), hex, 40);
            snprintf(buf, sizeof(buf), "Vendor:%s", hex);
            break;
        }
        case 8: snprintf(buf, sizeof(buf), "按OK重新捕获"); break;
    }

    u8g2.drawStr(2, 24, buf);

    // Page indicator
    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "第%d/8页  上下", cursor + 1);
    u8g2.drawStr(0, 63, buf);
}
