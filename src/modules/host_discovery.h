#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <ESP8266WiFi.h>

#define HOST_MAX 40

struct HostInfo {
    IPAddress ip;
    uint8_t  mac[6];
    char     hostname[32];
    bool     online;
};

class HostDiscovery : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "Host Discovery"; }
    const char* getTitle() const override    { return "Host Discovery"; }
    const unsigned char* getIcon() const override { return icon_host; }
    uint8_t     getId() const override       { return 3; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    HostInfo hosts[HOST_MAX];
    uint8_t  hostCount;
    uint8_t  cursor;
    uint8_t  scanIndex;
    uint32_t lastCheck;
    bool     scanning;
    IPAddress subnetBase;
    IPAddress subnetMask;

    void startScan();
    bool pingIP(IPAddress ip);
    void arpLookup(IPAddress ip);
};

ModuleInterface* createHostDiscovery();
