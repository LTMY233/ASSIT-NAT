#pragma once
#include "module_interface.h"
#include "../icons.h"

class ChannelHopper : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName() const override     { return "信道跳频器"; }
    const char* getTitle() const override    { return "信道跳频器"; }
    const unsigned char* getIcon() const override { return icon_ch_switch; }
    uint8_t     getId() const override       { return 26; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    uint8_t  currentCh;
    int8_t   rssi;
    uint32_t lastHop;
    uint16_t hopInterval;  // ms
    bool     running;
    uint8_t  speedLevel;   // 0-4

    void hopChannel();
    uint16_t getSpeedMs() const;
};

ModuleInterface* createChannelHopper();
