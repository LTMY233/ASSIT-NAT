#pragma once
#include "module_interface.h"
#include "../icons.h"

class RssiDistance : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 1; }
    const char* getName() const override     { return "RSSI Distance"; }
    const char* getTitle() const override    { return "RSSI Distance"; }
    const unsigned char* getIcon() const override { return icon_distance; }
    uint8_t     getId() const override       { return 22; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    float   n;          // path loss exponent (default 2.5)
    int8_t  refRssi;    // 1m reference RSSI (default -40)
    int8_t  currentRssi;// current target RSSI
    float   distance;   // estimated distance (m)
    uint8_t editMode;   // 0=view, 1=edit n, 2=edit ref

    void calcDistance();
};

ModuleInterface* createRssiDistance();
