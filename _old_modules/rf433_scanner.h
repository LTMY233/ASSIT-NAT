#pragma once
#include "module_interface.h"
#include "../icons.h"

#define RF433_SCAN_MAX_PULSES 64

class Rf433Scanner : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName() const override     { return "433MHz Scanner"; }
    const char* getTitle() const override    { return "433MHz Scanner"; }
    uint8_t     getId() const override       { return 72; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    unsigned long lastValue;
    uint8_t        lastBitLength;
    uint8_t        lastProtocol;
    int            rawPulses[RF433_SCAN_MAX_PULSES];
    uint8_t        pulseCount;
    uint16_t       avgPulseUs;
    uint32_t       signalCount;
    bool           hasData;
    bool           running;

    void analyzePulses();
    const char* guessProtocol();
};

ModuleInterface* createRf433Scanner();
