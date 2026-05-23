#pragma once
#include "module_interface.h"
#include "../icons.h"

#define SPECTRUM_SCAN_INTERVAL 3000

class SpectrumView : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName() const override     { return "频谱视图"; }
    const char* getTitle() const override    { return "频谱视图"; }
    const unsigned char* getIcon() const override { return icon_channel; }
    uint8_t     getId() const override       { return 25; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    int8_t   chRssi[14];     // RSSI per channel 1-13
    uint8_t  maxRssi;
    uint32_t lastScan;
    bool     scanning;
    bool     autoRefresh;

    void doScan();
};

ModuleInterface* createSpectrumView();
