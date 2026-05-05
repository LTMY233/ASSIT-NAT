#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

class EvilTwin : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 2; }
    const char* getName() const override     { return "Evil Twin"; }
    const char* getTitle() const override    { return "Evil Twin + Captive Portal"; }
    const unsigned char* getIcon() const override { return icon_eviltwin; }
    uint8_t     getId() const override       { return 46; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    bool     active;
    bool     confirmed;
    uint32_t startTime;
    char     targetSsid[33];
    uint8_t  targetBssid[6];
    uint8_t  targetChannel;
    uint8_t  connectedClients;
    uint8_t  phishingHits;

    void startAttack();
    void stopAttack();
    void setupDns();
    void setupWebServer();
};

ModuleInterface* createEvilTwin();
