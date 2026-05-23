#pragma once
#include "module_interface.h"
#include <ESP8266WiFi.h>

class DnsLookup : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "DNS查询"; }
    const char* getTitle() const override    { return "DNS查询"; }
    uint8_t     getId() const override       { return 11; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    char     hostname[48];
    uint8_t  editPos;
    uint8_t  hostLen;
    IPAddress resolvedIP;
    bool     resolved;
    bool     resolving;
    char     resultStr[24];

    void doLookup();
    static char cycleChar(char c, bool up);
};

ModuleInterface* createDnsLookup();
