#include "../chinese_glyphs.h"
#include "wps_pin.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createWpsPinCalc() { return new WpsPinCalc(); }

void WpsPinCalc::init() {
    mac[0] = 0x04; mac[1] = 0xCF; mac[2] = 0x8C;
    mac[3] = 0x00; mac[4] = 0x00; mac[5] = 0x00;
    editPos = 0;
    pin[0] = '\0';
    computed = false;
    macStr[0] = '\0';
}

void WpsPinCalc::enter() {
    editPos = 0;
    computed = false;
    pin[0] = '\0';
    displayMgr.setDirty();
}

void WpsPinCalc::exit() {}

uint32_t WpsPinCalc::wpsChecksum(uint32_t val) {
    uint32_t accum = 0;
    accum += (val / 10000000) % 10 * 3;
    accum += (val / 1000000) % 10;
    accum += (val / 100000) % 10 * 3;
    accum += (val / 10000) % 10;
    accum += (val / 1000) % 10 * 3;
    accum += (val / 100) % 10;
    accum += (val / 10) % 10 * 3;
    accum += val % 10;
    uint32_t digit = accum % 10;
    return (10 - digit) % 10;
}

void WpsPinCalc::computePin() {
    // WPS PIN calculation from MAC address
    // PIN = first 7 digits derived from MAC, 8th is checksum
    uint32_t val = 0;
    // Use last 3 MAC bytes
    val = ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | mac[5];
    uint32_t pinVal = val % 10000000;
    uint32_t checksum = wpsChecksum(pinVal);
    uint32_t fullPin = pinVal * 10 + checksum;
    snprintf(pin, sizeof(pin), "%08lu", (unsigned long)fullPin);

    macToStr(mac, macStr, sizeof(macStr));
    computed = true;
}

void WpsPinCalc::update() {}

void WpsPinCalc::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            computePin();
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            mac[editPos]++;
            computed = false;
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            mac[editPos]--;
            computed = false;
            displayMgr.setDirty();
            break;
        case BTN_OK_LONG:
            editPos = (editPos + 1) % 6;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void WpsPinCalc::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // MAC display
    u8g2.setFont(FONT_BIG);
    char macBuf[18];
    snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X",
             mac[0], mac[1], mac[2]);
    uint8_t tw = u8g2.getStrWidth(macBuf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, macBuf);

    u8g2.setFont(FONT_DATA);
    char restBuf[10];
    snprintf(restBuf, sizeof(restBuf), ":%02X:%02X:%02X", mac[3], mac[4], mac[5]);
    tw = u8g2.getStrWidth(restBuf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 36, restBuf);

    if (computed) {
        // PIN output
        u8g2.setFont(FONT_BIG);
        tw = u8g2.getStrWidth(pin);
        u8g2.drawStr((OLED_WIDTH - tw) / 2, 52, pin);

        u8g2.setFont(FONT_DATA);
        drawCN(u8g2, 0, 63, "WPS PIN 8位");
    } else {
        u8g2.setFont(FONT_DATA);
        drawCN(u8g2, 2, 48, "按OK计算PIN");
    }
}
