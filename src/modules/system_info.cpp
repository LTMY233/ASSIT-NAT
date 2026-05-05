#include "system_info.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include <ESP8266WiFi.h>

ModuleInterface* createSystemInfo() { return new SystemInfo(); }

void SystemInfo::init() {
    freeHeap = 0; totalHeap = 0; flashSize = 0;
    cpuFreq = 0; uptimeSec = 0;
    macStr[0] = '\0'; ipStr[0] = '\0'; sdkVer[0] = '\0';
    lastRefresh = 0;
}

void SystemInfo::enter() {
    refreshData();
    lastRefresh = millis();
    displayMgr.setDirty();
}

void SystemInfo::exit() {}

void SystemInfo::refreshData() {
    freeHeap = ESP.getFreeHeap();
    totalHeap = 0; // ESP8266 doesn't have getTotalHeap, approximate
    flashSize = ESP.getFlashChipRealSize();
    cpuFreq = ESP.getCpuFreqMHz();
    uptimeSec = millis() / 1000;
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    IPAddress ip = WiFi.localIP();
    snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    snprintf(sdkVer, sizeof(sdkVer), "%s", ESP.getSdkVersion());
}

void SystemInfo::update() {
    if (millis() - lastRefresh > 2000) {
        refreshData();
        lastRefresh = millis();
        displayMgr.setDirty();
    }
}

void SystemInfo::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        refreshData();
        lastRefresh = millis();
        displayMgr.setDirty();
    }
}

void SystemInfo::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "System Info");

    u8g2.setFont(FONT_BODY);
    char buf[64];

    // Heap
    snprintf(buf, sizeof(buf), "Heap:%u/%u", freeHeap, totalHeap);
    u8g2.drawStr(2, 24, buf);

    // Flash
    snprintf(buf, sizeof(buf), "Flash:%uKB", flashSize / 1024);
    u8g2.drawStr(2, 33, buf);

    // CPU freq
    snprintf(buf, sizeof(buf), "CPU:%uMHz", cpuFreq);
    u8g2.drawStr(70, 33, buf);

    // Uptime
    uint32_t h = uptimeSec / 3600;
    uint32_t m = (uptimeSec % 3600) / 60;
    uint32_t s = uptimeSec % 60;
    snprintf(buf, sizeof(buf), "Up:%02u:%02u:%02u", h, m, s);
    u8g2.drawStr(2, 42, buf);

    // MAC
    u8g2.drawStr(2, 51, macStr);

    // IP & SDK
    snprintf(buf, sizeof(buf), "%s SDK:%s", ipStr, sdkVer);
    u8g2.drawStr(0, 63, buf);
}
