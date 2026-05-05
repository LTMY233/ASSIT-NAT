#pragma once
#include "module_interface.h"
#include "../icons.h"

class RssiHistogram : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 1; }
    const char* getName() const override     { return "RSSI Histogram"; }
    const char* getTitle() const override    { return "RSSI Histogram"; }
    const unsigned char* getIcon() const override { return icon_histogram; }
    uint8_t     getId() const override       { return 24; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    // Bins: -30~-39, -40~-49, ..., -80~-89, <-90
    uint8_t bins[7];
    uint8_t maxBin;
    bool    hasData;
    uint32_t lastScan;

    void doScan();
};

ModuleInterface* createRssiHistogram();
