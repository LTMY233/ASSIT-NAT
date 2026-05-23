#pragma once
#include "module_interface.h"

class BatteryAdc : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 7; }
    const char* getName() const override     { return "电池电压"; }
    const char* getTitle() const override    { return "电池监测"; }
    uint8_t     getId() const override       { return 84; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    uint16_t rawValue;
    float    voltage_meas;  // measured voltage at ADC (0-1V)
    float    batteryVolts;  // estimated battery voltage (after divider)
    uint8_t  batteryPercent;
    uint32_t lastRead;
    float    batMin;        // minimum battery voltage
    float    batMax;        // maximum battery voltage

    void readBattery();
};

ModuleInterface* createBatteryAdc();
