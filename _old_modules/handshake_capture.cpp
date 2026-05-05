#include "handshake_capture.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"

HandshakeCapture* HandshakeCapture::instance = nullptr;

ModuleInterface* createHandshakeCapture() { return new HandshakeCapture(); }

void HandshakeCapture::init() {
    running = false; captured = false;
    memset(handshake, 0, sizeof(handshake));
    captureStart = 0; pcapLen = 0;
    memset(targetBssid, 0, 6);
    memset(pcapBuf, 0, sizeof(pcapBuf));
    instance = this;
}

void HandshakeCapture::enter() {
    running = false; captured = false;
    memset(handshake, 0, sizeof(handshake));
    captureStart = millis(); pcapLen = 0;
    instance = this;
    wifiMgr.promiscuousAcquire(getId());
    writePcapHeader();
    running = true;
    displayMgr.setDirty();
}

void HandshakeCapture::exit() {
    running = false;
    wifiMgr.promiscuousRelease(getId());
    instance = nullptr;
}

void HandshakeCapture::writePcapHeader() {
    // pcap global header
    uint32_t magic = 0xA1B2C3D4;
    uint16_t major = 2, minor = 4;
    int32_t tz = 0; uint32_t sigfigs = 0;
    uint32_t snapLen = 65535;
    uint32_t linkType = 105;  // 802.11

    memcpy(pcapBuf, &magic, 4);
    memcpy(pcapBuf + 4, &major, 2);
    memcpy(pcapBuf + 6, &minor, 2);
    memcpy(pcapBuf + 8, &tz, 4);
    memcpy(pcapBuf + 12, &sigfigs, 4);
    memcpy(pcapBuf + 16, &snapLen, 4);
    memcpy(pcapBuf + 20, &linkType, 4);
    pcapLen = 24;
}

void HandshakeCapture::appendPcapFrame(uint8_t* buf, uint16_t len) {
    if (pcapLen + 16 + len > sizeof(pcapBuf)) return;
    uint32_t tsSec = millis() / 1000;
    uint32_t tsUsec = (millis() % 1000) * 1000;

    uint32_t frameLen = len;
    memcpy(pcapBuf + pcapLen, &tsSec, 4);
    memcpy(pcapBuf + pcapLen + 4, &tsUsec, 4);
    memcpy(pcapBuf + pcapLen + 8, &frameLen, 4);
    memcpy(pcapBuf + pcapLen + 12, &frameLen, 4);
    memcpy(pcapBuf + pcapLen + 16, buf, len);
    pcapLen += 16 + len;
}

void HandshakeCapture::onPacket(uint8_t* buf, uint16_t len) {
    if (!instance || instance->captured || len < 24) return;
    instance->processEapol(buf, len);
}

void HandshakeCapture::processEapol(uint8_t* buf, uint16_t len) {
    uint8_t fc = buf[0];
    if ((fc & 0x0C) != 0x08) return;  // not data frame

    uint8_t* llc = buf + 24;
    if (len < 32) return;
    if (llc[0] != 0xAA || llc[1] != 0xAA || llc[2] != 0x03) return;
    if (llc[6] != 0x88 || llc[7] != 0x8E) return;  // EAPOL EtherType

    uint8_t* eapol = llc + 8;
    if (len - 32 < 4) return;
    uint8_t keyType = eapol[0];  // 1=RC4, 2=RSN
    uint8_t keyInfo = eapol[5];

    // WPA 4-way handshake Key Info flags:
    // M1: Key ACK=1, MIC=0, Secure=0, from AP
    // M2: Key ACK=0, MIC=1, Secure=0, from STA
    // M3: Key ACK=1, MIC=1, Secure=1, from AP
    // M4: Key ACK=0, MIC=1, Secure=1, from STA

    uint8_t msgNum = 0;
    if (keyInfo & 0x80) {  // Key ACK
        if (keyInfo & 0x01) msgNum = (keyInfo & 0x02) ? 3 : 1;
        else msgNum = 1;
    } else {
        msgNum = (keyInfo & 0x01) ? 4 : 2;
    }

    if (msgNum >= 1 && msgNum <= 4) {
        handshake[msgNum - 1] = 1;
        memcpy(targetBssid, buf + 10, 6);
        appendPcapFrame(buf, len);

        // Check if all 4 frames collected
        captured = (handshake[0] && handshake[1] && handshake[2] && handshake[3]);
        if (captured) {
            lfsWriteFile("/handshake.pcap", pcapBuf, pcapLen);
        }
        displayMgr.setDirty();
    }
}

void HandshakeCapture::update() {
    if (!running) return;
    // 60-sec timeout
    if (!captured && millis() - captureStart > 60000) {
        running = false;
        wifiMgr.promiscuousRelease(getId());
        displayMgr.setDirty();
    }
}

void HandshakeCapture::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        if (captured) {
            captured = false; memset(handshake, 0, sizeof(handshake));
            captureStart = millis(); pcapLen = 24;  // restart capture
            displayMgr.setDirty();
        }
    }
}

void HandshakeCapture::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Handshake Capture");
    u8g2.setFont(FONT_DATA);

    if (captured) {
        u8g2.drawStr(2, 24, "Full handshake captured!");
        u8g2.drawStr(2, 39, "Saved to /handshake.pcap");
        u8g2.drawStr(2, 54, "Press OK to recapture");
    } else if (running) {
        u8g2.drawStr(2, 24, "Listening handshake...");
        char buf[30];
        snprintf(buf, sizeof(buf), "M1:%c M2:%c M3:%c M4:%c",
                 handshake[0] ? 'O' : '-',
                 handshake[1] ? 'O' : '-',
                 handshake[2] ? 'O' : '-',
                 handshake[3] ? 'O' : '-');
        u8g2.drawStr(2, 39, buf);
        uint32_t elapsed = (millis() - captureStart) / 1000;
        snprintf(buf, sizeof(buf), "Time:%lus/60s", elapsed);
        u8g2.drawStr(2, 54, buf);
    } else {
        u8g2.drawStr(10, 35, "Timeout - re-enter module");
    }
}
