#include "traceroute.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <WiFiUdp.h>
#include <ESP8266Ping.h>

ModuleInterface* createTraceroute() { return new Traceroute(); }

void Traceroute::init() {
    memset(hops, 0, sizeof(hops));
    hopCount = 0; currentTtl = 1;
    tracing = false; lastProbe = 0;
}

void Traceroute::enter() {
    hopCount = 0; currentTtl = 1;
    targetIP = IPAddress(8, 8, 8, 8);  // default Google DNS
    lastProbe = millis();
    tracing = false;
    startTrace(targetIP);
    displayMgr.setDirty();
}

void Traceroute::exit() {}

void Traceroute::startTrace(IPAddress target) {
    targetIP = target;
    tracing = true;
    hopCount = 0;
    currentTtl = 1;
    lastProbe = millis();
    sendProbe(currentTtl);
}

void Traceroute::sendProbe(uint8_t ttl) {
    // ESP8266Ping does not support setting TTL directly
    // Use simplified UDP traceroute method
    IPAddress ip;
    // Send high-port UDP packet, TTL incrementing
    WiFiUDP udp;
    udp.begin(33434 + ttl);
    udp.beginPacket(targetIP, 33434);
    udp.write((uint8_t*)"TRACE", 5);
    udp.endPacket();
    udp.stop();
    lastProbe = millis();
}

bool Traceroute::checkReply() {
    // Simplified: use ping instead
    return false;
}

void Traceroute::update() {
    if (!tracing) return;

    // Simplified: one ping per hop
    // Full traceroute requires raw socket support, not fully on ESP8266
    // This provides basic framework, displays per-hop probe via ping lib

    uint32_t now = millis();
    if (now - lastProbe > 2000 && currentTtl < TR_MAX_HOPS) {
        hops[hopCount].ttl = currentTtl;
        hops[hopCount].ip = targetIP;
        hops[hopCount].rtt = -1;  // Simplified: cannot get intermediate hop
        hopCount++;
        currentTtl++;

        if (currentTtl > TR_MAX_HOPS) {
            tracing = false;
        }
        lastProbe = now;
        displayMgr.setDirty();
    }
}

void Traceroute::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT && !tracing) { startTrace(targetIP); displayMgr.setDirty(); }
}

void Traceroute::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Traceroute");

    if (tracing) {
        u8g2.setFont(FONT_BODY);
        char buf[20];
        snprintf(buf, sizeof(buf), "Tracing... TTL=%d", currentTtl);
        u8g2.drawStr(2, 25, buf);
    }

    u8g2.setFont(FONT_DATA);
    for (uint8_t i = 0; i < hopCount && i < 2; i++) {
        uint8_t y = 24 + i * 15;
        char buf[32];
        if (hops[i].rtt < 0) {
            snprintf(buf, sizeof(buf), "%2d  *", hops[i].ttl);
        } else {
            snprintf(buf, sizeof(buf), "%2d  %d.%d.%d.%d  %dms",
                     hops[i].ttl, hops[i].ip[0], hops[i].ip[1],
                     hops[i].ip[2], hops[i].ip[3], hops[i].rtt);
        }
        u8g2.drawStr(2, y, buf);
    }

    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 63, "Full trace needs Raw Socket");
}
