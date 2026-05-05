#include "pmkid_capture.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"

PmkidCapture* PmkidCapture::instance = nullptr;

ModuleInterface* createPmkidCapture() { return new PmkidCapture(); }

void PmkidCapture::init() {
    running = false; captured = false;
    captureTime = 0; pmkidLen = 0;
    memset(pmkidData, 0, sizeof(pmkidData));
    memset(targetBssid, 0, 6);
    instance = this;
}

void PmkidCapture::enter() {
    captured = false; pmkidLen = 0; instance = this;
    wifiMgr.promiscuousAcquire(getId());
    running = true;
    displayMgr.setDirty();
}

void PmkidCapture::exit() {
    running = false;
    wifiMgr.promiscuousRelease(getId());
    instance = nullptr;
}

void PmkidCapture::onPacket(uint8_t* buf, uint16_t len) {
    if (!instance || instance->captured || len < 24) return;
    instance->parseEapol(buf, len);
}

void PmkidCapture::parseEapol(uint8_t* buf, uint16_t len) {
    // Check if EAPOL frame
    uint8_t fc = buf[0];
    if ((fc & 0x0C) != 0x08) return;  // not data frame

    // LLC/SNAP header
    uint8_t* llc = buf + 24;
    if (len < 32) return;
    if (llc[0] != 0xAA || llc[1] != 0xAA || llc[2] != 0x03) return;

    // EAPOL type
    if (llc[6] != 0x88 || llc[7] != 0x8E) return;

    // Extract frame body
    uint8_t* body = llc + 8;
    uint16_t bodyLen = len - 32;

    // Check RSN IE for PMKID
    for (uint16_t i = 0; i < bodyLen - 2; ) {
        uint8_t elemId = body[i];
        uint8_t elemLen = body[i + 1];
        if (elemId == 48 && elemLen >= 22) {  // RSN
            // Check if contains PMKID
            uint16_t pmkidCount = ((uint16_t)body[i + 20] << 8) | body[i + 21];
            if (pmkidCount > 0 && i + 22 + pmkidCount * 16 <= bodyLen) {
                // Save PMKID and source MAC
                memcpy(targetBssid, buf + 10, 6);  // SA
                instance->pmkidLen = min((uint8_t)(pmkidCount * 16), (uint8_t)120);
                memcpy(instance->pmkidData, body + i + 22, instance->pmkidLen);
                instance->captured = true;
                instance->captureTime = millis();
                return;
            }
        }
        i += 2 + elemLen;
    }
}

void PmkidCapture::update() {
    if (!running) return;
    if (captured) displayMgr.setDirty();
}

void PmkidCapture::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT && captured) {
        // Save to LittleFS
        uint8_t saveBuf[6 + 1 + 128] = {0};
        memcpy(saveBuf, targetBssid, 6);
        saveBuf[6] = pmkidLen;
        memcpy(saveBuf + 7, pmkidData, pmkidLen);
        lfsWriteFile("/pmkid.dat", saveBuf, 7 + pmkidLen);
        captured = false; pmkidLen = 0;
        displayMgr.setDirty();
    }
}

void PmkidCapture::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "PMKID Capture");
    u8g2.setFont(FONT_DATA);

    if (captured) {
        char macBuf[18]; macToStr(targetBssid, macBuf, sizeof(macBuf));
        u8g2.drawStr(2, 24, "PMKID Captured!");
        u8g2.drawStr(2, 39, macBuf);
        u8g2.drawStr(2, 54, "Press OK to save");
    } else if (running) {
        u8g2.drawStr(10, 35, "Listening EAPOL...");
    }
}
