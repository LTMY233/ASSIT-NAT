#pragma once
#include "module_interface.h"
#include "../icons.h"

class Deauth : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 2; }
    const char* getName() const override     { return "Deauth Test"; }
    const char* getTitle() const override    { return "Deauth Injector"; }
    const unsigned char* getIcon() const override { return icon_deauth; }
    uint8_t     getId() const override       { return 40; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    uint8_t  targetBssid[6];
    uint8_t  targetClient[6];  // FF:FF:FF:FF:FF:FF = broadcast
    uint8_t  packetCount;
    bool     attacking;
    bool     confirmed;   // double confirm
    uint32_t attackStart;
    uint8_t  sentCount;
    uint32_t lastSent;

    void startAttack();
    void stopAttack();
    void sendDeauthFrame();
};

ModuleInterface* createDeauth();
