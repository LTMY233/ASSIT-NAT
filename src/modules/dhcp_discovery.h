#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define DHCP_MAX_SERVERS 5

struct DhcpInfo {
    IPAddress ip;
    IPAddress offeredSubnet;
    IPAddress offeredRouter;
    uint32_t leaseTime;
};

class DhcpDiscovery : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "DHCP发现"; }
    const char* getTitle() const override    { return "DHCP发现"; }
    const unsigned char* getIcon() const override { return icon_dhcp; }
    uint8_t     getId() const override       { return 8; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    DhcpInfo servers[DHCP_MAX_SERVERS];
    uint8_t  serverCount;
    bool     scanning;
    uint32_t scanStart;
    WiFiUDP  udp;

    void sendDiscover();
    void parseOffer(uint8_t* buf, uint16_t len);
};

ModuleInterface* createDhcpDiscovery();
