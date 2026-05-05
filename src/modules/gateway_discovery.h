#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <ESP8266WiFi.h>

class GatewayDiscovery : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 5; }
    const char* getName() const override     { return "Gateway Discovery"; }
    const char* getTitle() const override    { return "Gateway Discovery"; }
    const unsigned char* getIcon() const override { return icon_gw_disc; }
    uint8_t     getId() const override       { return 60; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    IPAddress gateway;
    uint8_t  gatewayMac[6];
    bool     hasMac;
    int16_t  pingRtt;
    bool     pingOk;
};

ModuleInterface* createGatewayDiscovery();
