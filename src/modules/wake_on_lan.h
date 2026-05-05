#pragma once
#include "module_interface.h"
#include <ESP8266WiFi.h>

class WakeOnLan : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "Wake On LAN"; }
    const char* getTitle() const override    { return "Wake On LAN"; }
    uint8_t     getId() const override       { return 15; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint8_t  mac[6];
    uint8_t  editPos;
    bool     sent;
    char     status[20];

    void sendMagicPacket();
};

ModuleInterface* createWakeOnLan();
