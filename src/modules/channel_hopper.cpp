#include "channel_hopper.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>

ModuleInterface* createChannelHopper() { return new ChannelHopper(); }

void ChannelHopper::init() {
    currentCh = 1;
    rssi = -100;
    lastHop = 0;
    hopInterval = 500;
    running = false;
    speedLevel = 2;  // middle speed
}

void ChannelHopper::enter() {
    currentCh = 1;
    rssi = -100;
    lastHop = millis();
    running = true;
    speedLevel = 2;
    hopInterval = getSpeedMs();
    wifi_promiscuous_enable(false);
    hopChannel();
    displayMgr.setDirty();
}

void ChannelHopper::exit() {
    running = false;
    wifi_promiscuous_enable(false);
}

void ChannelHopper::hopChannel() {
    wifi_set_channel(currentCh);
    // Quick passive RSSI sample
    int8_t bestRssi = -100;
    // Do a quick scan on this channel
    WiFi.scanNetworks(true, false);
    lastHop = millis();
}

void ChannelHopper::update() {
    if (!running) return;

    // Check scan result
    int n = WiFi.scanComplete();
    if (n >= 0) {
        int8_t bestRssi = -100;
        for (int i = 0; i < n; i++) {
            if (WiFi.channel(i) == currentCh) {
                if (WiFi.RSSI(i) > bestRssi) bestRssi = WiFi.RSSI(i);
            }
        }
        rssi = bestRssi;
        WiFi.scanDelete();
        displayMgr.setDirty();
    }

    // Hop to next channel
    if (millis() - lastHop > hopInterval) {
        currentCh++;
        if (currentCh > 13) currentCh = 1;
        hopChannel();
        // Clear RSSI until scan completes
        rssi = -100;
        lastHop = millis();
        displayMgr.setDirty();
    }
}

uint16_t ChannelHopper::getSpeedMs() const {
    // speedLevel 0=2s, 1=1s, 2=500ms, 3=250ms, 4=100ms
    switch (speedLevel) {
        case 0: return 2000;
        case 1: return 1000;
        case 2: return 500;
        case 3: return 250;
        case 4: return 100;
        default: return 500;
    }
}

void ChannelHopper::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            running = !running;
            if (running) {
                lastHop = millis();
                hopChannel();
            }
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            if (speedLevel < 4) speedLevel++;
            hopInterval = getSpeedMs();
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (speedLevel > 0) speedLevel--;
            hopInterval = getSpeedMs();
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void ChannelHopper::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Channel Hopper");

    // Current channel (big)
    u8g2.setFont(FONT_BIG);
    char buf[20];
    snprintf(buf, sizeof(buf), "CH%d", currentCh);
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, buf);

    // RSSI bar
    u8g2.setFont(FONT_DATA);
    if (rssi > -100) {
        snprintf(buf, sizeof(buf), "RSSI:%d dBm", rssi);
        u8g2.drawStr(2, 40, buf);

        // RSSI bar: -95..-30 maps to 0..100 width
        uint8_t barW = mapRange(rssi, -95, -30, 0, 100);
        if (barW > 100) barW = 100;
        u8g2.drawFrame(14, 44, 100, 6);
        if (barW > 0) {
            u8g2.drawBox(14, 44, barW, 6);
        }
    } else {
        u8g2.drawStr(2, 40, "RSSI:--- dBm");
    }

    // Speed indicator
    snprintf(buf, sizeof(buf), "Speed:%dms %s",
             hopInterval, running ? "RUN" : "STOP");
    u8g2.drawStr(2, 54, buf);

    // Footer
    u8g2.drawStr(0, 63, "OK:run UP/DN:speed");
}
