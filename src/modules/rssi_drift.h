#pragma once
#include "module_interface.h"
#include "../icons.h"

#define DRIFT_SAMPLES 64

class RssiDrift : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 5; }
    const char* getName() const override     { return "RSSI Drift"; }
    const char* getTitle() const override    { return "RSSI Drift"; }
    const unsigned char* getIcon() const override { return icon_drift; }
    uint8_t     getId() const override       { return 63; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    int8_t   samples[DRIFT_SAMPLES];
    uint8_t  sampleIdx;
    uint8_t  sampleCount;
    int8_t   currentRssi;
    int8_t   minRssi, maxRssi;
    uint32_t lastSample;
};

ModuleInterface* createRssiDrift();
