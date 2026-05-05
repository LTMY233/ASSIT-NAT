#pragma once
#include <U8g2lib.h>
#include "config.h"
#include "modules/module_interface.h"

class DisplayMgr {
public:
    void init();

    // Mark dirty for redraw
    void setDirty();
    bool isDirty() const { return dirty; }

    // Call each loop, render-on-demand
    void update(SystemState sysState, ModuleInterface* activeModule = nullptr);

    // Get U8g2 instance
    U8G2& getU8g2() { return u8g2; }

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2{U8G2_R0, U8X8_PIN_NONE};
    bool  dirty;
    uint32_t lastRender;

    void drawSplash();
};

extern DisplayMgr displayMgr;
