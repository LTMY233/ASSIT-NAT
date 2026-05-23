#include "probe_sniffer.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../core/sw_rtc.h"
#include "../utils.h"
#include "../chinese_glyphs.h"
#include <ESP8266WiFi.h>

ProbeSniffer* ProbeSniffer::instance = nullptr;

ModuleInterface* createProbeSniffer() { return new ProbeSniffer(); }

// 802.11 management frame structure (simplified)
struct MgmtFrameHdr {
    uint8_t  frameCtrl[2];
    uint8_t  duration[2];
    uint8_t  addr1[6];  // RA
    uint8_t  addr2[6];  // TA (source MAC)
    uint8_t  addr3[6];  // BSSID
    uint8_t  seqCtrl[2];
};

void ProbeSniffer::init() {
    memset(devices, 0, sizeof(devices));
    deviceCount = 0;
    lastCleanup = 0;
    cursor = 0;
    running = false;
    instance = this;
}

void ProbeSniffer::enter() {
    deviceCount = 0;
    cursor = 0;
    lastCleanup = millis();
    instance = this;
    wifiMgr.promiscuousAcquire(getId());
    displayMgr.setDirty();
    running = true;
}

void ProbeSniffer::exit() {
    running = false;
    wifiMgr.promiscuousRelease(getId());
    instance = nullptr;
}

void ProbeSniffer::onPacket(uint8_t* buf, uint16_t len) {
    if (!instance || len < 24) return;

    MgmtFrameHdr* hdr = (MgmtFrameHdr*)buf;
    uint8_t type = hdr->frameCtrl[0] & 0x0C;  // mgmt/ctrl/data
    uint8_t subtype = (hdr->frameCtrl[0] >> 4) & 0x0F;

    // Probe Req: mgmt frame (type=0), subtype 0x04
    if (type != 0x00 || subtype != 0x04) return;

    int8_t rssi = -(256 - buf[len - 1]);  // signal strength in last byte

    // Extract SSID
    uint8_t* body = buf + 24;
    uint16_t bodyLen = len - 24 - 4;  // minus FCS
    char ssid[33] = "";
    for (uint16_t i = 0; i < bodyLen && i < 200; ) {
        uint8_t elemId = body[i];
        uint8_t elemLen = body[i + 1];
        if (elemId == 0 && elemLen > 0 && elemLen <= 32) {
            memcpy(ssid, body + i + 2, elemLen);
            ssid[elemLen] = '\0';
            break;
        }
        i += 2 + elemLen;
    }

    // Search/update in device list
    for (uint8_t i = 0; i < instance->deviceCount; i++) {
        if (memcmp(instance->devices[i].mac, hdr->addr2, 6) == 0) {
            instance->devices[i].rssi = rssi;
            instance->devices[i].lastSeen = millis();
            if (ssid[0]) strCopySafe(instance->devices[i].lastSsid, ssid, 33);
            return;
        }
    }

    // new device
    if (instance->deviceCount < PROBE_MAX_DEVICES) {
        ProbeDevice& d = instance->devices[instance->deviceCount++];
        memcpy(d.mac, hdr->addr2, 6);
        d.rssi = rssi;
        strCopySafe(d.lastSsid, ssid, 33);
        d.lastSeen = millis();
    }
}

void ProbeSniffer::update() {
    if (!running) return;
    uint32_t now = millis();

    // Cleanup inactive every 30s
    if (now - lastCleanup > 30000) {
        lastCleanup = now;
        for (int i = deviceCount - 1; i >= 0; i--) {
            if (now - devices[i].lastSeen > 120000) {
                memmove(&devices[i], &devices[i + 1],
                        (deviceCount - i - 1) * sizeof(ProbeDevice));
                deviceCount--;
            }
        }
        displayMgr.setDirty();
    }
}

void ProbeSniffer::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT && cursor > 0) { cursor--; displayMgr.setDirty(); }
    if (ev == BTN_DOWN_SHORT && cursor < deviceCount - 1) { cursor++; displayMgr.setDirty(); }
}

void ProbeSniffer::draw(U8G2& u8g2) {
    drawCN(u8g2, 0, 10, "探针嗅探器");

    if (deviceCount == 0) {
        drawCN(u8g2, 4, 38, "正在监听探针...");
        return;
    }

    u8g2.setFont(FONT_DATA);
    u8g2.drawHLine(0, 14, OLED_WIDTH);
    u8g2.drawStr(0, 24, "MAC");
    u8g2.drawStr(82, 24, "dBm");
    u8g2.drawStr(105, 24, "SSID");

    uint8_t show = (deviceCount < 2) ? deviceCount : 2;

    for (uint8_t i = 0; i < show && i < deviceCount; i++) {
        uint8_t y = 39 + i * 15;
        if (i == cursor) {
            u8g2.drawBox(0, y - 12, OLED_WIDTH, 15);
            u8g2.setDrawColor(0);
        }

        char macBuf[18]; macToStr(devices[i].mac, macBuf, sizeof(macBuf));
        u8g2.drawStr(0, y, macBuf);

        char rssiBuf[5]; itoaSimple(devices[i].rssi, rssiBuf, 10);
        u8g2.drawStr(82, y, rssiBuf);

        u8g2.drawStr(105, y, devices[i].lastSsid[0] ? devices[i].lastSsid : "?");

        if (i == cursor) u8g2.setDrawColor(1);
    }

    u8g2.setFont(FONT_DATA);
    char cntBuf[20];
    snprintf(cntBuf, sizeof(cntBuf), "设备: %d", deviceCount);
    u8g2.drawStr(0, 63, cntBuf);
}
