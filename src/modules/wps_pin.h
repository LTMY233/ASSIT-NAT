#pragma once
#include "module_interface.h"

class WpsPinCalc : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 3; }
    const char* getName() const override     { return "WPS PIN Calc"; }
    const char* getTitle() const override    { return "WPS Pin Calc"; }
    uint8_t     getId() const override       { return 48; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }
    bool        canRunOffline() const override { return true; }

private:
    uint8_t  mac[6];
    uint8_t  editPos;
    char     pin[12];
    bool     computed;
    char     macStr[18];

    void computePin();
    static uint32_t wpsChecksum(uint32_t val);
};

ModuleInterface* createWpsPinCalc();
