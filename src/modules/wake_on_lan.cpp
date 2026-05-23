#include "../chinese_glyphs.h"
#include "wake_on_lan.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <WiFiUdp.h>

ModuleInterface* createWakeOnLan() { return new WakeOnLan(); }

void WakeOnLan::init() {
    memset(mac, 0, sizeof(mac));
    mac[0] = 0x00; mac[1] = 0x11; mac[2] = 0x22;
    mac[3] = 0x33; mac[4] = 0x44; mac[5] = 0x55;
    editPos = 0;
    sent = false;
    strCopySafe(status, "OK=send", sizeof(status));
}

void WakeOnLan::enter() {
    editPos = 0;
    sent = false;
    strCopySafe(status, "OK=send", sizeof(status));
    displayMgr.setDirty();
}

void WakeOnLan::exit() {}

void WakeOnLan::sendMagicPacket() {
    WiFiUDP udp;
    udp.begin(0);

    uint8_t packet[102];
    // 6 bytes of 0xFF
    memset(packet, 0xFF, 6);
    // 16 repetitions of MAC address
    for (uint8_t i = 0; i < 16; i++) {
        memcpy(packet + 6 + i * 6, mac, 6);
    }

    // Send to broadcast
    IPAddress broadcast = WiFi.localIP();
    broadcast[3] = 255;

    udp.beginPacket(broadcast, 9);
    udp.write(packet, 102);
    udp.endPacket();

    // Also send to 255.255.255.255
    IPAddress bcast(255, 255, 255, 255);
    udp.beginPacket(bcast, 9);
    udp.write(packet, 102);
    udp.endPacket();

    // Send one more on port 7
    udp.beginPacket(broadcast, 7);
    udp.write(packet, 102);
    udp.endPacket();

    udp.stop();
    sent = true;
    strCopySafe(status, "Sent!", sizeof(status));
}

void WakeOnLan::update() {}

void WakeOnLan::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            sendMagicPacket();
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            mac[editPos]++;
            sent = false;
            strCopySafe(status, "OK=send", sizeof(status));
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            mac[editPos]--;
            sent = false;
            strCopySafe(status, "OK=send", sizeof(status));
            displayMgr.setDirty();
            break;
        case BTN_OK_LONG:
            editPos = (editPos + 1) % 6;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void WakeOnLan::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // MAC address display
    u8g2.setFont(FONT_BIG);
    char macBuf[18];
    snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X",
             mac[0], mac[1], mac[2]);
    uint8_t tw = u8g2.getStrWidth(macBuf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, macBuf);

    u8g2.setFont(FONT_DATA);
    char restBuf[10];
    snprintf(restBuf, sizeof(restBuf), ":%02X:%02X:%02X", mac[3], mac[4], mac[5]);
    tw = u8g2.getStrWidth(restBuf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 36, restBuf);

    // Edit indicator
    u8g2.setFont(FONT_MONO);
    char editHint[4] = {' ', ' ', ' ', '\0'};
    editHint[editPos] = '^';
    u8g2.drawStr((OLED_WIDTH - u8g2.getStrWidth("^")) / 2, 44, editHint);

    if (sent) {
        drawCN(u8g2, 20, 55, "已发送!");
    } else {
        drawCN(u8g2, 0, 55, "按OK发送唤醒包");
    }
}
