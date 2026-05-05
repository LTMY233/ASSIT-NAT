#pragma once
#include "module_interface.h"
#include "../icons.h"

class SubnetCalc : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 2; }
    const char* getName() const override     { return "Subnet Calc"; }
    const char* getTitle() const override    { return "Subnet Calc"; }
    const unsigned char* getIcon() const override { return icon_host; }
    uint8_t     getId() const override       { return 69; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint8_t ip[4];
    uint8_t mask[4];
    uint8_t network[4];
    uint8_t broadcast[4];
    uint8_t firstHost[4];
    uint8_t lastHost[4];
    uint8_t editOctet;   // 0-3 = editing ip[0]-ip[3], 4-7 = mask[0]-mask[3]
    bool    editingMask;
    uint8_t cidr;

    void recalc();
    uint8_t maskToCidr();
    void cidrToMask(uint8_t c);
};

ModuleInterface* createSubnetCalc();
