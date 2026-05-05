#pragma once
#include "module_interface.h"
#include "../icons.h"

class MacLookup : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 3; }
    const char* getName() const override     { return "MAC Lookup"; }
    const char* getTitle() const override    { return "MAC OUI Lookup"; }
    const unsigned char* getIcon() const override { return icon_mac; }
    uint8_t     getId() const override       { return 32; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint8_t  mac[6];
    uint8_t  editPos;   // current edit pos (0-5)
    char     vendorName[48];
    bool     found;

    void lookup();
};

ModuleInterface* createMacLookup();
