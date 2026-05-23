#pragma once
#include "module_interface.h"

enum ScreenPattern : uint8_t {
    PAT_ALL_ON       = 0,
    PAT_ALL_OFF      = 1,
    PAT_CHECKERBOARD = 2,
    PAT_H_LINES      = 3,
    PAT_V_LINES      = 4,
    PAT_BORDER       = 5,
    PAT_CROSSHAIR    = 6,
    PAT_COUNT        = 7
};

class ScreenTest : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 7; }
    const char* getName() const override     { return "屏幕测试"; }
    const char* getTitle() const override    { return "屏幕测试"; }
    uint8_t     getId() const override       { return 82; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint8_t currentPattern;

    const char* getPatternName(uint8_t p);
    void drawPattern(U8G2& u8g2, uint8_t p);
};

ModuleInterface* createScreenTest();
