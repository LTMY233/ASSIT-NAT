#pragma once
#include "module_interface.h"
#include "../icons.h"

class LogicProbe : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 4; }
    const char* getName() const override     { return "Logic Probe"; }
    const char* getTitle() const override    { return "Logic Probe"; }
    const unsigned char* getIcon() const override { return icon_probe; }
    uint8_t     getId() const override       { return 50; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    uint8_t  pin;
    bool     level;
    uint32_t lastRead;
    uint32_t highCount, lowCount;
    uint32_t transitions;
};

ModuleInterface* createLogicProbe();
