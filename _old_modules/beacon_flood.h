#pragma once
#include "module_interface.h"
#include "../icons.h"

class BeaconFlood : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 2; }
    const char* getName() const override     { return "Beacon Flood"; }
    const char* getTitle() const override    { return "Beacon Flood Gen"; }
    const unsigned char* getIcon() const override { return icon_flood; }
    uint8_t     getId() const override       { return 41; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    bool     attacking;
    bool     confirmed;
    uint32_t attackStart;
    uint32_t lastBeacon;
    uint8_t  ssidIndex;
    bool     wantsStatusBar() const override { return true; }

    static const char* fakeSsidList[];
    static uint8_t fakeSsidCount;

    void startAttack();
    void stopAttack();
    void sendBeacon(const char* ssid);
};

ModuleInterface* createBeaconFlood();
