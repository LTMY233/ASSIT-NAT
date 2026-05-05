#include "rogue_ap.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"

ModuleInterface* createRogueAp() { return new RogueAp(); }

void RogueAp::init() {
    memset(clients, 0, sizeof(clients));
    clientCount = 0; running = false;
    startTime = 0; confirmed = false;
}

void RogueAp::enter() {
    clientCount = 0; running = false;
    confirmed = false; startTime = 0;
    displayMgr.setDirty();
}

void RogueAp::exit() {
    if (running) stopAp();
}

void RogueAp::startAp() {
    if (running) return;
    wifiMgr.startSoftAP("FreeWiFi", nullptr, 6);  // open AP no password
    running = true;
    startTime = millis();
}

void RogueAp::stopAp() {
    running = false;
    confirmed = false;
    wifiMgr.stopSoftAP();
    wifiMgr.setStationMode();
    displayMgr.setDirty();
}

void RogueAp::update() {
    if (!running) return;

    // Hard time limit
    if (millis() - startTime > SEC_ATTACK_MAX_DURATION) {
        stopAp();
        return;
    }

    // Check connected devices
    struct station_info* stInfo = wifi_softap_get_station_info();
    while (stInfo) {
        // Check if already recorded
        bool found = false;
        for (uint8_t i = 0; i < clientCount; i++) {
            if (memcmp(clients[i].mac, stInfo->bssid, 6) == 0) { found = true; break; }
        }
        if (!found && clientCount < ROGUE_MAX_CLIENTS) {
            memcpy(clients[clientCount].mac, stInfo->bssid, 6);
            clients[clientCount].connectTime = millis();
            clientCount++;
            displayMgr.setDirty();
        }
        stInfo = STAILQ_NEXT(stInfo, next);
    }
    wifi_softap_free_station_info();
}

void RogueAp::handleButton(ButtonEvent ev) {
    if (running) {
        if (ev != BTN_NONE) stopAp();
        return;
    }
    if (!confirmed) {
        if (ev == BTN_OK_SHORT) { confirmed = true; displayMgr.setDirty(); }
    } else {
        if (ev == BTN_OK_SHORT) { startAp(); displayMgr.setDirty(); }
    }
}

void RogueAp::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Rogue AP");
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 24, "⚠ Test Mode - Own Devices Only ⚠");

    if (running) {
        uint32_t remaining = (SEC_ATTACK_MAX_DURATION - (millis() - startTime)) / 1000;
        char buf[30];
        snprintf(buf, sizeof(buf), "SSID:FreeWiFi left:%lus", remaining);
        u8g2.drawStr(2, 39, buf);

        if (clientCount > 0) {
            char macBuf[18]; macToStr(clients[0].mac, macBuf, sizeof(macBuf));
            u8g2.drawStr(2, 54, macBuf);
        }
        snprintf(buf, sizeof(buf), "Found:%d clients", clientCount);
        u8g2.drawStr(2, 63, buf);
    } else if (!confirmed) {
        u8g2.drawStr(2, 39, "Press OK to start open AP");
    } else {
        u8g2.drawStr(2, 39, "Press OK to start");
    }
}
