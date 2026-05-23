#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <WiFiUdp.h>

#define UPNP_MAX_DEVICES 15

struct UpnpDevice {
    char location[64];
    char server[32];
    char usn[48];
    char st[32];  // Search Target type
};

class UpnpDiscovery : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "UPnP发现"; }
    const char* getTitle() const override    { return "UPnP发现"; }
    const unsigned char* getIcon() const override { return icon_upnp; }
    uint8_t     getId() const override       { return 10; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    UpnpDevice devices[UPNP_MAX_DEVICES];
    uint8_t  deviceCount;
    bool     scanning;
    uint32_t scanStart;
    WiFiUDP  udp;
    uint8_t  cursor;

    void sendMsearch();
    void parseResponse(uint8_t* buf, uint16_t len);
};

ModuleInterface* createUpnpDiscovery();
