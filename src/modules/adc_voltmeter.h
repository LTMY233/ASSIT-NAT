#pragma once
#include "module_interface.h"
#include "../icons.h"

class AdcVoltmeter : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 4; }
    const char* getName() const override     { return "ADC电压表"; }
    const char* getTitle() const override    { return "ADC电压表"; }
    const unsigned char* getIcon() const override { return icon_adc; }
    uint8_t     getId() const override       { return 52; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    uint16_t rawValue;
    float    voltage;
    uint32_t lastRead;
};

ModuleInterface* createAdcVoltmeter();
