#pragma once
#include "module_interface.h"
#include "../icons.h"

class FreqGenerator : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 4; }
    const char* getName() const override     { return "频率发生器"; }
    const char* getTitle() const override    { return "频率发生器"; }
    const unsigned char* getIcon() const override { return icon_pwm; }
    uint8_t     getId() const override       { return 57; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint32_t frequency;    // Hz
    uint8_t  dutyCycle;    // 0-100%
    uint8_t  waveform;     // 0=SQUARE, 1=SINE, 2=TRIANGLE, 3=SAWTOOTH
    bool     output;
    uint8_t  editMode;     // 0=freq, 1=duty, 2=wave

    void applySignal();
};

ModuleInterface* createFreqGenerator();
