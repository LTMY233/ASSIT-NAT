#pragma once
#include "module_interface.h"
#include "../icons.h"

class RfNoise : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName() const override     { return "射频噪声"; }
    const char* getTitle() const override    { return "射频噪声"; }
    const unsigned char* getIcon() const override { return icon_quality; }
    uint8_t     getId() const override       { return 28; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    uint8_t  currentCh;
    int8_t   noiseFloor;   // RSSI with no AP around
    int8_t   chNoise[14];  // stored per channel
    uint32_t lastSample;
    uint8_t  samplesCollected;
    bool     measuring;

    void measureNoise();
    int8_t getMinRssiOnChannel();
};

ModuleInterface* createRfNoise();
