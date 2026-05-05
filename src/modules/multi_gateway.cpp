#include "multi_gateway.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"

MultiGateway* MultiGateway::instance = nullptr;

ModuleInterface* createMultiGateway() { return new MultiGateway(); }

void MultiGateway::init() {
    memset(gateways, 0, sizeof(gateways));
    gwCount = 0; alert = false;
    startTime = 0; running = false;
    instance = this;
}

void MultiGateway::enter() {
    memset(gateways, 0, sizeof(gateways));
    gwCount = 0; alert = false;
    startTime = millis(); instance = this;
    wifiMgr.promiscuousAcquire(getId());
    running = true;
    displayMgr.setDirty();
}

void MultiGateway::exit() {
    running = false;
    wifiMgr.promiscuousRelease(getId());
    instance = nullptr;
}

void MultiGateway::onPacket(uint8_t* buf, uint16_t len) {
    if (!instance || len < 42) return;

    // Check if ARP packet (in data frame)
    uint8_t* llc = buf + 24;
    if (llc[0] != 0xAA || llc[1] != 0xAA || llc[2] != 0x03) return;
    if (llc[6] != 0x08 || llc[7] != 0x06) return;  // ARP EtherType

    uint8_t* arp = llc + 8;
    if (arp[7] != 0x02) return;  // ARP Reply

    // Sender IP (arp + 14)
    IPAddress senderIp(arp[14], arp[15], arp[16], arp[17]);
    uint8_t* senderMac = arp + 8;

    // Check if gateway IP (simplified: same /24 subnet)
    if (senderIp[3] == 1) {  // usually .1 = gateway
        bool exists = false;
        for (uint8_t i = 0; i < instance->gwCount; i++) {
            if (memcmp(instance->gateways[i].mac, senderMac, 6) == 0) {
                instance->gateways[i].lastSeen = millis();
                exists = true; break;
            }
        }
        if (!exists && instance->gwCount < 4) {
            memcpy(instance->gateways[instance->gwCount].mac, senderMac, 6);
            instance->gateways[instance->gwCount].firstSeen = millis();
            instance->gateways[instance->gwCount].lastSeen = millis();
            instance->gwCount++;
            if (instance->gwCount > 1) instance->alert = true;
        }
    }
}

void MultiGateway::update() {
    if (!running) return;
    if (gwCount > 1 && alert) displayMgr.setDirty();
}

void MultiGateway::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        memset(gateways, 0, sizeof(gateways));
        gwCount = 0; alert = false;
        startTime = millis();
        displayMgr.setDirty();
    }
}

void MultiGateway::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "ARP Gateway Spoof Detection");

    if (alert) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(2, 24, "⚠ Multiple gateway MACs!");
    }

    u8g2.setFont(FONT_DATA);
    for (uint8_t i = 0; i < gwCount; i++) {
        uint8_t y = 39 + i * 15;
        char macBuf[18]; macToStr(gateways[i].mac, macBuf, sizeof(macBuf));
        u8g2.drawStr(2, y, macBuf);
    }

    if (gwCount <= 1) u8g2.drawStr(10, 39, "Normal - single gateway");
    u8g2.drawStr(0, 63, "OK to reset");
}
