#include "i2c_scanner.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createI2cScanner() { return new I2cScanner(); }

void I2cScanner::init() {
    memset(addresses, 0, sizeof(addresses));
    devCount = 0; scanning = false;
    scanAddr = 1; lastScan = 0;
}

void I2cScanner::enter() {
    devCount = 0; scanning = false;
    scanAddr = 1; lastScan = millis();
    scanning = true;
    displayMgr.setDirty();
}

void I2cScanner::exit() {}

void I2cScanner::update() {
    if (!scanning) return;

    if (scanAddr <= 127 && millis() - lastScan > 5) {
        Wire.beginTransmission(scanAddr);
        if (Wire.endTransmission() == 0) {
            if (devCount < I2C_MAX_DEVICES) {
                addresses[devCount++] = scanAddr;
            }
        }
        scanAddr++;
        lastScan = millis();
        displayMgr.setDirty();
    }

    if (scanAddr > 127) {
        scanning = false;
        displayMgr.setDirty();
    }
}

void I2cScanner::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT && !scanning) {
        devCount = 0; scanAddr = 1;
        scanning = true; lastScan = millis();
        displayMgr.setDirty();
    }
}

void I2cScanner::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "I2C Scanner");

    if (scanning) {
        u8g2.setFont(FONT_BODY);
        char buf[20];
        snprintf(buf, sizeof(buf), "Scan 0x%02X/0x7F", scanAddr);
        u8g2.drawStr(2, 30, buf);
        uint8_t prog = scanAddr * OLED_WIDTH / 127;
        u8g2.drawFrame(0, 38, OLED_WIDTH, 5);
        u8g2.drawBox(0, 38, prog, 5);
    } else if (devCount == 0) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(10, 35, "No I2C devices");
    } else {
        u8g2.setFont(FONT_BODY);
        for (uint8_t i = 0; i < devCount; i++) {
            char buf[8];
            snprintf(buf, sizeof(buf), "0x%02X", addresses[i]);
            u8g2.drawStr(10 + (i % 4) * 30, 25 + (i / 4) * 12, buf);
        }
    }

    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 63, scanning ? "" : "OK to rescan");
}
