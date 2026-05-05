#include "net_snapshot.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/sw_rtc.h"
#include "../utils.h"
#include <ESP8266WiFi.h>

ModuleInterface* createNetSnapshot() { return new NetSnapshot(); }

void NetSnapshot::init() {
    saved = false; filename[0] = '\0';
    saveTime = 0; apCount = 0; avgRssi = 0;
}

void NetSnapshot::enter() {
    saved = false; filename[0] = '\0';
    displayMgr.setDirty();
}

void NetSnapshot::exit() {}

void NetSnapshot::saveSnapshot() {
    // Scan and save results
    int n = WiFi.scanNetworks();
    if (n < 0) return;

    apCount = n;
    int32_t rssiSum = 0;

    char txt[1024] = {0};
    char* p = txt;
    p += snprintf(p, sizeof(txt) - (p - txt), "Net Snapshot\n");
    p += snprintf(p, sizeof(txt) - (p - txt), "================\n");

    char timeBuf[24]; formatDateTime(swRTC.now(), timeBuf, sizeof(timeBuf));
    p += snprintf(p, sizeof(txt) - (p - txt), "Time: %s\n", timeBuf);
    p += snprintf(p, sizeof(txt) - (p - txt), "APs: %d\n\n", n);

    for (int i = 0; i < n && (p - txt) < 900; i++) {
        rssiSum += WiFi.RSSI(i);
        p += snprintf(p, sizeof(txt) - (p - txt), "CH:%2d  %4ddBm  %s\n",
                      WiFi.channel(i), WiFi.RSSI(i), WiFi.SSID(i).c_str());
    }

    if (n > 0) avgRssi = rssiSum / n;

    // Generate filename
    snprintf(filename, sizeof(filename), "/snapshot_%lu.txt", (unsigned long)millis());

    lfsWriteFile(filename, (uint8_t*)txt, strlen(txt));
    saveTime = millis();
    saved = true;
}

void NetSnapshot::update() {}

void NetSnapshot::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT && !saved) {
        saveSnapshot();
        displayMgr.setDirty();
    } else if (ev == BTN_OK_SHORT && saved) {
        saved = false;
        displayMgr.setDirty();
    }
}

void NetSnapshot::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Network Snapshot");

    if (saved) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(2, 25, "Snapshot saved!");
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(2, 35, filename);
        char buf[30];
        snprintf(buf, sizeof(buf), "AP:%d RSSI:%d avg", apCount, avgRssi);
        u8g2.drawStr(2, 45, buf);
        u8g2.drawStr(0, 63, "OK for new snapshot");
    } else {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(10, 35, "OK to save snapshot");
    }
}
