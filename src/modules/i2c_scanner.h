#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <Wire.h>

#define I2C_MAX_DEVICES 10

class I2cScanner : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 4; }
    const char* getName() const override     { return "I2C Scanner"; }
    const char* getTitle() const override    { return "I2C Scanner"; }
    const unsigned char* getIcon() const override { return icon_i2c; }
    uint8_t     getId() const override       { return 53; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint8_t  addresses[I2C_MAX_DEVICES];
    uint8_t  devCount;
    bool     scanning;
    uint8_t  scanAddr;
    uint32_t lastScan;
};

ModuleInterface* createI2cScanner();
