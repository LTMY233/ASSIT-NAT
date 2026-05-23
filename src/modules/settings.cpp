#include "../chinese_glyphs.h"
#include "settings.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createSettings() { return new Settings(); }

void Settings::init() {
    page = 0;
    contrast = 128;
    inverted = false;
    cursor = 0;
}

void Settings::enter() {
    page = 0;
    cursor = 0;
    displayMgr.setDirty();
}

void Settings::exit() {
}

void Settings::update() {
}

void Settings::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_UP_SHORT:
            if (page == 0 && contrast < 250) {
                contrast += 10;
                displayMgr.getU8g2().setContrast(contrast);
            }
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (page == 0 && contrast > 10) {
                contrast -= 10;
                displayMgr.getU8g2().setContrast(contrast);
            }
            displayMgr.setDirty();
            break;
        case BTN_OK_SHORT:
            page = (page + 1) % PAGE_COUNT;
            displayMgr.setDirty();
            break;
        case BTN_OK_DOUBLE:
            page = 0;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void Settings::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    switch (page) {
        case 0: {
            drawCN(u8g2, 0, 28, "亮度");
            u8g2.drawFrame(0, 34, OLED_WIDTH, 10);
            uint8_t barW = contrast * (OLED_WIDTH - 2) / 255;
            u8g2.drawBox(1, 35, barW, 8);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", contrast);
            u8g2.drawStr(50, 24, buf);
            u8g2.setFont(FONT_BODY);
            u8g2.drawStr(0, 56, "上下调节  OK换页");
            break;
        }
        case 1: {
            u8g2.drawStr(0, 24, "ESP8266 Toolbox");
            u8g2.drawStr(0, 38, "固件 v1.0");
            u8g2.drawStr(0, 50, "编译: 2026-05-05");
            char buf[32];
            snprintf(buf, sizeof(buf), "可用内存: %d", ESP.getFreeHeap());
            u8g2.drawStr(0, 63, buf);
            break;
        }
        case 2: {
            drawCN(u8g2, 0, 24, "作者");
            u8g2.drawStr(0, 40, "ltmy_233");
            drawCN(u8g2, 0, 56, "瑞士军刀");
            break;
        }
    }

    u8g2.setFont(FONT_DATA);
    char pbuf[16];
    snprintf(pbuf, sizeof(pbuf), "%d/%d", page + 1, PAGE_COUNT);
    u8g2.drawStr(OLED_WIDTH - 24, 63, pbuf);
}
