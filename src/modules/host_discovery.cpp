#include "host_discovery.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/button_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>

ModuleInterface* createHostDiscovery() { return new HostDiscovery(); }

void HostDiscovery::init() {
    memset(hosts, 0, sizeof(hosts));
    hostCount = 0;
    cursor = 0;
    scanIndex = 0;
    lastCheck = 0;
    scanning = false;
}

void HostDiscovery::enter() {
    hostCount = 0;
    cursor = 0;
    subnetBase = WiFi.localIP();
    subnetMask = WiFi.subnetMask();
    startScan();
    displayMgr.setDirty();
}

void HostDiscovery::exit() {}

void HostDiscovery::startScan() {
    scanning = true;
    scanIndex = 1;  // start at .1 (usually gateway)
    hostCount = 0;
    lastCheck = millis();
}

bool HostDiscovery::pingIP(IPAddress ip) {
    return Ping.ping(ip) > 0;
}

void HostDiscovery::arpLookup(IPAddress ip) {
    // ESP8266 has no direct ARP lookup API
    // Try reverse lookup via WiFi.hostByName
    // Simple ping result logging
}

void HostDiscovery::update() {
    if (!scanning) return;
    uint32_t now = millis();

    if (now - lastCheck > 50 && scanIndex < 255) {
        IPAddress target = subnetBase;
        target[3] = scanIndex;
        lastCheck = now;

        // Process buttons during scan so long-press exit works
        buttonMgr.update();
        if (buttonMgr.hasEvents()) {
            ButtonEvent ev;
            buttonMgr.getEvent(ev);
            if (ev == BTN_OK_LONG || ev == BTN_OK_DOUBLE) {
                scanning = false;
                return;
            }
        }

        if (pingIP(target)) {
            if (hostCount < HOST_MAX) {
                hosts[hostCount].ip = target;
                hosts[hostCount].online = true;
                hostCount++;
            }
        }
        scanIndex++;
        displayMgr.setDirty();
    }

    if (scanIndex >= 254) {
        scanning = false;
        displayMgr.setDirty();
    }
}

void HostDiscovery::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT && cursor > 0) { cursor--; displayMgr.setDirty(); }
    if (ev == BTN_DOWN_SHORT && cursor < hostCount - 1) { cursor++; displayMgr.setDirty(); }
    if (ev == BTN_OK_SHORT && !scanning) { startScan(); displayMgr.setDirty(); }
}

void HostDiscovery::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Host Discovery");

    if (scanning) {
        u8g2.setFont(FONT_BODY);
        char buf[32];
        IPAddress cur = subnetBase; cur[3] = scanIndex;
        char ipBuf[16]; ipToStr(cur, ipBuf, sizeof(ipBuf));
        snprintf(buf, sizeof(buf), "Scan: %s (%d/254)", ipBuf, scanIndex);
        u8g2.drawStr(2, 30, buf);
        // progress bar
        uint8_t prog = scanIndex * OLED_WIDTH / 254;
        u8g2.drawFrame(0, 38, OLED_WIDTH, 5);
        u8g2.drawBox(0, 38, prog, 5);
        snprintf(buf, sizeof(buf), "Found: %d", hostCount);
        u8g2.drawStr(2, 50, buf);
        return;
    }

    if (hostCount == 0) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(10, 35, "No devices found");
        return;
    }

    u8g2.setFont(FONT_DATA);
    for (uint8_t i = 0; i < hostCount && i < 2; i++) {
        uint8_t y = 39 + i * 15;
        if (i == cursor) {
            u8g2.drawBox(0, y - 12, OLED_WIDTH, 15);
            u8g2.setDrawColor(0);
        }
        char ipBuf[16]; ipToStr(hosts[i].ip, ipBuf, sizeof(ipBuf));
        u8g2.drawStr(0, y, ipBuf);
        if (i == cursor) u8g2.setDrawColor(1);
    }

    u8g2.setFont(FONT_DATA);
    char buf[20];
    snprintf(buf, sizeof(buf), "%d devices online", hostCount);
    u8g2.drawStr(0, 63, buf);
}
