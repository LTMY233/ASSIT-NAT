#include "display_mgr.h"
#include "config.h"
#include "menu_engine.h"
#include <Wire.h>

DisplayMgr displayMgr;

void DisplayMgr::init() {
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    Wire.setClock(I2C_SPEED_HZ);
    u8g2.begin();
    u8g2.setContrast(128);

    dirty = true;
    lastRender = 0;
}

void DisplayMgr::setDirty() {
    dirty = true;
}

void DisplayMgr::drawSplash() {
    u8g2.setFont(FONT_BIG);
    const char* title = "ASSIT-NAT";
    uint8_t tw = u8g2.getStrWidth(title);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 22, title);

    u8g2.setFont(FONT_BODY);
    const char* sub = "Swiss Army Knife";
    tw = u8g2.getStrWidth(sub);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 36, sub);

    u8g2.setFont(FONT_TITLE);
    const char* author = "by:ltmy_233";
    tw = u8g2.getStrWidth(author);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 52, author);
}

void DisplayMgr::update(SystemState sysState, ModuleInterface* activeModule) {
    if (activeModule && activeModule->getRefreshMode() == REFRESH_CONTINUOUS) {
        dirty = true;
    }

    if (!dirty) return;
    dirty = false;
    lastRender = millis();

    if (sysState == STATE_BOOT) {
        u8g2.clearBuffer();
        drawSplash();
        u8g2.sendBuffer();
    } else if (sysState == STATE_TOOL_RUNNING && activeModule) {
        u8g2.clearBuffer();
        activeModule->draw(u8g2);
        u8g2.sendBuffer();
    } else {
        u8g2.clearBuffer();
        menuEngine.draw(u8g2);
        u8g2.sendBuffer();
    }
}
