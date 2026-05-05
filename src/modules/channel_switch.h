#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <ESP8266WiFi.h>

class ChannelSwitch : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 5; }
    const char* getName() const override     { return "Ch Switch Time"; }
    const char* getTitle() const override    { return "Ch Switch Time"; }
    const unsigned char* getIcon() const override { return icon_ch_switch; }
    uint8_t     getId() const override       { return 62; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint8_t  fromChannel, toChannel;
    uint32_t switchTime;    // us
    bool     measured;
    uint32_t startUs;
    uint8_t  phase;  // 0=idle, 1=switching, 2=done
};

ModuleInterface* createChannelSwitch();
