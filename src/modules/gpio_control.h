#pragma once
#include "module_interface.h"
#include "../icons.h"

#define GPIO_PIN_COUNT 9    // D0-D8

class GpioControl : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 4; }
    const char* getName() const override     { return "GPIO Control"; }
    const char* getTitle() const override    { return "GPIO Control"; }
    const unsigned char* getIcon() const override { return icon_probe; }
    uint8_t     getId() const override       { return 54; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint8_t  selectedPin;    // 0-8 maps to D0-D8
    bool     pinMode_[GPIO_PIN_COUNT];   // true=OUTPUT, false=INPUT
    bool     pinState_[GPIO_PIN_COUNT];  // HIGH/LOW state
    bool     editMode;       // false=select pin, true=edit state

    static const uint8_t pinMap[GPIO_PIN_COUNT];
    void readPin(uint8_t idx);
    void writePin(uint8_t idx, bool state);
};

ModuleInterface* createGpioControl();
