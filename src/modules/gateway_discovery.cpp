#include "gateway_discovery.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <ESP8266Ping.h>

ModuleInterface* createGatewayDiscovery() { return new GatewayDiscovery(); }

void GatewayDiscovery::init() {
    gateway = IPAddress(0,0,0,0);
    memset(gatewayMac, 0, 6); hasMac = false;
    pingRtt = -1; pingOk = false;
}

void GatewayDiscovery::enter() {
    gateway = WiFi.gatewayIP();
    hasMac = false; pingRtt = -1; pingOk = false;

    // Send ping test
    pingOk = Ping.ping(gateway, 1);

    // MAC needs ARP table
    // ESP8266 has no direct ARP table API
    // Simplify: show known info
    hasMac = false;
    displayMgr.setDirty();
}

void GatewayDiscovery::exit() {}

void GatewayDiscovery::update() {}

void GatewayDiscovery::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        pingOk = Ping.ping(gateway, 1);
        displayMgr.setDirty();
    }
}

void GatewayDiscovery::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Gateway Discovery");

    u8g2.setFont(FONT_BODY);
    char buf[40];
    snprintf(buf, sizeof(buf), "IP: %d.%d.%d.%d",
             gateway[0], gateway[1], gateway[2], gateway[3]);
    u8g2.drawStr(2, 24, buf);

    u8g2.setFont(FONT_DATA);
    if (hasMac) {
        char macBuf[18]; macToStr(gatewayMac, macBuf, sizeof(macBuf));
        snprintf(buf, sizeof(buf), "MAC:%s", macBuf);
        u8g2.drawStr(2, 39, buf);
    }

    snprintf(buf, sizeof(buf), "Ping:%s RTT=%dms",
             pingOk ? "OK" : "FAIL", pingOk ? pingRtt : -1);
    u8g2.drawStr(2, 54, buf);

    u8g2.drawStr(0, 63, "OK to re-ping");
}
