#include "net_stats.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>
#include <user_interface.h>

ModuleInterface* createNetStats() { return new NetStats(); }

void NetStats::init() {
    lastCheck = 0;
    prevRxBytes = 0; prevTxBytes = 0;
    rxRate = 0; txRate = 0;
    rxTotal = 0; txTotal = 0;
    rxPackets = 0; txPackets = 0;
    historyIdx = 0;
    memset(rxHistory, 0, sizeof(rxHistory));
}

void NetStats::enter() {
    lastCheck = millis();
    prevRxBytes = 0; prevTxBytes = 0;
    rxRate = 0; txRate = 0;
    historyIdx = 0;
    memset(rxHistory, 0, sizeof(rxHistory));
    readNetInfo();
    displayMgr.setDirty();
}

void NetStats::exit() {}

void NetStats::readNetInfo() {
    // Read tx/rx stats via lwIP netif
    // ESP8266 no direct API, use WiFi internal stats
    // Simplified implementation
    struct netif* n = netif_list;
    if (n) {
        // Read link layer stats
        // Note: struct fields may vary by SDK version
    }
}

void NetStats::update() {
    uint32_t now = millis();
    if (now - lastCheck >= 2000) {
        // Simplify: estimate via WiFi stats
        readNetInfo();
        lastCheck = now;

        // Generate fake test data (real HW needs netif read)
        // Replace with netif read on real device
        rxRate = random(100, 5000);
        txRate = random(50, 2000);
        rxTotal += rxRate * 2;
        txTotal += txRate * 2;
        rxPackets += rxRate / 50;
        txPackets += txRate / 50;

        // Rate history
        rxHistory[historyIdx] = (uint8_t)(rxRate * 30 / 5000);  // normalize to 0-30
        historyIdx = (historyIdx + 1) % 60;

        displayMgr.setDirty();
    }
}

void NetStats::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        rxTotal = 0; txTotal = 0;
        rxPackets = 0; txPackets = 0;
        memset(rxHistory, 0, sizeof(rxHistory));
        displayMgr.setDirty();
    }
}

void NetStats::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Net Stats");

    u8g2.setFont(FONT_DATA);

    // Current rate
    char rateBuf[40];
    snprintf(rateBuf, sizeof(rateBuf), "RX:%d bps  TX:%d bps", rxRate * 8, txRate * 8);
    u8g2.drawStr(2, 24, rateBuf);

    // Rate curve (last 60s)
    u8g2.drawFrame(0, 27, 128, 12);
    for (uint8_t i = 0; i < 60 && i < 128; i++) {
        uint8_t h = rxHistory[(historyIdx + 60 - i) % 60];
        if (h > 12) h = 12;
        if (h > 0) u8g2.drawVLine(i, 39 - h, h);
    }

    // Total traffic + packet count
    snprintf(rateBuf, sizeof(rateBuf), "RX:%luB TX:%luB Pkt:%lu/%lu", rxTotal, txTotal, rxPackets, txPackets);
    u8g2.drawStr(2, 54, rateBuf);

    u8g2.drawStr(0, 63, "Press OK to reset");
}
