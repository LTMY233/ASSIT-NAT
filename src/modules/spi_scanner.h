#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <SPI.h>

#define SPI_MAX_DEVICES 8

class SpiScanner : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 4; }
    const char* getName() const override     { return "SPI Scanner"; }
    const char* getTitle() const override    { return "SPI Scanner"; }
    const unsigned char* getIcon() const override { return icon_i2c; }
    uint8_t     getId() const override       { return 56; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint8_t  foundPins[SPI_MAX_DEVICES];
    uint8_t  devCount;
    bool     scanning;
    uint8_t  scanPin;
    uint32_t lastScan;

    static const uint8_t csPins[SPI_MAX_DEVICES];
    bool probeCs(uint8_t csPin);
};

ModuleInterface* createSpiScanner();
