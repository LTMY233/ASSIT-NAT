#include "../chinese_glyphs.h"
#include "screen_test.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createScreenTest() { return new ScreenTest(); }

void ScreenTest::init() {
    currentPattern = PAT_ALL_ON;
}

void ScreenTest::enter() {
    currentPattern = PAT_ALL_ON;
    displayMgr.setDirty();
}

void ScreenTest::exit() {}

void ScreenTest::update() {}

const char* ScreenTest::getPatternName(uint8_t p) {
    switch (p) {
        case PAT_ALL_ON:       return "All On";
        case PAT_ALL_OFF:      return "All Off";
        case PAT_CHECKERBOARD: return "Checker";
        case PAT_H_LINES:      return "H Lines";
        case PAT_V_LINES:      return "V Lines";
        case PAT_BORDER:       return "Border";
        case PAT_CROSSHAIR:    return "Cross";
        default: return "Unknown";
    }
}

void ScreenTest::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_UP_SHORT:
            if (currentPattern > 0) currentPattern--;
            else currentPattern = PAT_COUNT - 1;
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (currentPattern < PAT_COUNT - 1) currentPattern++;
            else currentPattern = 0;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void ScreenTest::drawPattern(U8G2& u8g2, uint8_t p) {
    switch (p) {
        case PAT_ALL_ON:
            for (uint8_t x = 0; x < OLED_WIDTH; x += 2) {
                for (uint8_t y = 0; y < OLED_HEIGHT; y += 2) {
                    u8g2.drawPixel(x, y);
                }
            }
            break;
        case PAT_ALL_OFF:
            // Nothing drawn = all pixels off
            break;
        case PAT_CHECKERBOARD:
            for (uint8_t y = 0; y < OLED_HEIGHT; y++) {
                for (uint8_t x = 0; x < OLED_WIDTH; x++) {
                    if (((x / 8) + (y / 8)) % 2 == 0) {
                        u8g2.drawPixel(x, y);
                    }
                }
            }
            break;
        case PAT_H_LINES:
            for (uint8_t y = 0; y < OLED_HEIGHT; y += 4) {
                u8g2.drawHLine(0, y, OLED_WIDTH);
            }
            break;
        case PAT_V_LINES:
            for (uint8_t x = 0; x < OLED_WIDTH; x += 4) {
                u8g2.drawVLine(x, 0, OLED_HEIGHT);
            }
            break;
        case PAT_BORDER:
            u8g2.drawFrame(0, 0, OLED_WIDTH, OLED_HEIGHT);
            u8g2.drawFrame(2, 2, OLED_WIDTH - 4, OLED_HEIGHT - 4);
            u8g2.drawFrame(4, 4, OLED_WIDTH - 8, OLED_HEIGHT - 8);
            break;
        case PAT_CROSSHAIR:
            u8g2.drawHLine(0, OLED_HEIGHT / 2, OLED_WIDTH);
            u8g2.drawVLine(OLED_WIDTH / 2, 0, OLED_HEIGHT);
            u8g2.drawFrame(0, 0, OLED_WIDTH, OLED_HEIGHT);
            break;
    }
}

void ScreenTest::draw(U8G2& u8g2) {
    // Draw pattern in background area (below title, above footer)
    uint8_t patternAreaTop = 10;
    uint8_t patternAreaBot = 55;
    uint8_t patternH = patternAreaBot - patternAreaTop;

    // Draw pattern in the content area
    drawPattern(u8g2, currentPattern);

    // Title overlay
    u8g2.setFont(FONT_DATA);

    // Pattern name
    u8g2.setFont(FONT_BODY);
    char buf[32];
    snprintf(buf, sizeof(buf), "Pat:%d/%d %s",
             currentPattern + 1, PAT_COUNT, getPatternName(currentPattern));
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 24, buf);

    u8g2.setFont(FONT_DATA);
}
