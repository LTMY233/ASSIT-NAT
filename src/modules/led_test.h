#pragma once
#include "module_interface.h"

enum LedPattern : uint8_t {
    LED_BLINK     = 0,
    LED_SOS       = 1,
    LED_FAST      = 2,
    LED_SLOW      = 3,
    LED_HEARTBEAT = 4,
    LED_PAT_COUNT = 5
};

class LedTest : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 7; }
    const char* getName() const override     { return "LED测试"; }
    const char* getTitle() const override    { return "LED测试"; }
    uint8_t     getId() const override       { return 87; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    uint8_t  currentPattern;
    bool     running;
    bool     ledState;
    uint32_t lastToggle;
    uint8_t  stepIndex;     // for SOS pattern
    uint32_t sosTimings[6]; // SOS: short, short, short, long, long, long

    uint32_t getPatternInterval(uint8_t p);
    void     updateLed();
    const char* getPatternName(uint8_t p);
};

ModuleInterface* createLedTest();
