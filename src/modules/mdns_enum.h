#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <WiFiUdp.h>

#define MDNS_MAX_SERVICES 20

struct MdnsService {
    char type[32];
    char name[32];
    uint16_t port;
};

class MdnsEnum : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "mDNS Enum"; }
    const char* getTitle() const override    { return "mDNS Enum"; }
    const unsigned char* getIcon() const override { return icon_mdns; }
    uint8_t     getId() const override       { return 9; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    MdnsService services[MDNS_MAX_SERVICES];
    uint8_t  serviceCount;
    bool     scanning;
    uint32_t scanStart;
    WiFiUDP  udp;
    uint8_t  cursor;

    void sendQuery();
    void parseResponse(uint8_t* buf, uint16_t len);
};

ModuleInterface* createMdnsEnum();
