#include "../chinese_glyphs.h"
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
    p += snprintf(p, sizeof(txt) - (p - txt), "网络快照\n");
    p += snprintf(p, sizeof(txt) - (p - txt), "================\n");

    char timeBuf[24]; formatDateTime(swRTC.now(), timeBuf, sizeof(timeBuf));
    p += snprintf(p, sizeof(txt) - (p - txt), "时间: %s\n", timeBuf);
    p += snprintf(p, sizeof(txt) - (p - txt), "AP数: %d\n\n", n);

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

    if (saved) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 2, 25, "快照已保存");
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(2, 35, filename);
        char buf[34];
        snprintf(buf, sizeof(buf), "AP:%d RSSI:%d 平均", apCount, avgRssi);
        drawCN(u8g2, 2, 45, buf);
        drawCN(u8g2, 0, 63, "按OK新建快照");
    } else {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 10, 35, "按OK保存快照");
    }
}
