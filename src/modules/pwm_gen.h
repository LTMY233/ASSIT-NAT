#pragma once
#include "module_interface.h"
#include "../icons.h"

class PwmGen : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 4; }
    const char* getName() const override     { return "PWM Generator"; }
    const char* getTitle() const override    { return "PWM Generator"; }
    const unsigned char* getIcon() const override { return icon_pwm; }
    uint8_t     getId() const override       { return 51; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint16_t frequency;   // Hz
    uint8_t  dutyCycle;   // 0-100%
    bool     output;
    uint8_t  editMode;    // 0=freq, 1=duty

    void applyPwm();
};

ModuleInterface* createPwmGen();
