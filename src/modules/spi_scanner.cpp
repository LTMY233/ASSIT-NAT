#include "../chinese_glyphs.h"
#include "spi_scanner.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createSpiScanner() { return new SpiScanner(); }

// CS pins to probe: D0=16, D1=5, D2=4, D3=0, D4=2, D5=14, D6=12, D7=13
const uint8_t SpiScanner::csPins[SPI_MAX_DEVICES] = {
    16, 5, 4, 0, 2, 14, 12, 13
};

void SpiScanner::init() {
    memset(foundPins, 0, sizeof(foundPins));
    devCount = 0;
    scanning = false;
    scanPin = 0;
    lastScan = 0;
}

void SpiScanner::enter() {
    memset(foundPins, 0, sizeof(foundPins));
    devCount = 0;
    scanning = false;
    scanPin = 0;
    lastScan = 0;
    devCount = 0;
    scanning = true;
    scanPin = 0;
    SPI.begin();
    // Set all CS pins HIGH initially
    for (uint8_t i = 0; i < SPI_MAX_DEVICES; i++) {
        pinMode(csPins[i], OUTPUT);
        digitalWrite(csPins[i], HIGH);
    }
    lastScan = millis();
    displayMgr.setDirty();
}

void SpiScanner::exit() {
    SPI.end();
    scanning = false;
}

bool SpiScanner::probeCs(uint8_t csPin) {
    // Pull CS low and try SPI transaction
    digitalWrite(csPin, LOW);
    delayMicroseconds(10);
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
    uint8_t dummy = SPI.transfer(0xFF);
    SPI.endTransaction();
    digitalWrite(csPin, HIGH);

    // If device responds, it usually pulls MISO - reading 0x00 or 0xFF
    // is inconclusive, but we check the response isn't always 0xFF
    return (dummy != 0xFF);
}

void SpiScanner::update() {
    if (!scanning) return;

    if (scanPin < SPI_MAX_DEVICES && millis() - lastScan > 10) {
        if (probeCs(csPins[scanPin])) {
            if (devCount < SPI_MAX_DEVICES) {
                foundPins[devCount++] = csPins[scanPin];
            }
        }
        scanPin++;
        lastScan = millis();
        displayMgr.setDirty();
    }

    if (scanPin >= SPI_MAX_DEVICES) {
        scanning = false;
        displayMgr.setDirty();
    }
}

void SpiScanner::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT && !scanning) {
        devCount = 0;
        scanPin = 0;
        scanning = true;
        lastScan = millis();
        displayMgr.setDirty();
    }
}

void SpiScanner::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    if (scanning) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 2, 30, "探测中...");
        uint8_t prog = scanPin * OLED_WIDTH / SPI_MAX_DEVICES;
        u8g2.drawFrame(0, 38, OLED_WIDTH, 5);
        u8g2.drawBox(0, 38, prog, 5);
    } else if (devCount == 0) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 3, 35, "未发现SPI设备");
    } else {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 2, 24, "CS引脚发现:");
        for (uint8_t i = 0; i < devCount; i++) {
            char buf[8];
            // Map GPIO back to Dx notation
            uint8_t gpio = foundPins[i];
            uint8_t dpin = 0xFF;
            static const uint8_t gpioMap[] = {16, 5, 4, 0, 2, 14, 12, 13, 15};
            static const uint8_t dLabels[]   = {0,  1, 2, 3, 4,  5,  6,  7,  8};
            for (uint8_t j = 0; j < 9; j++) {
                if (gpioMap[j] == gpio) { dpin = j; break; }
            }
            snprintf(buf, sizeof(buf), "D%d", dpin);
            u8g2.drawStr(10 + (i % 4) * 30, 36 + (i / 4) * 10, buf);
        }
    }

    u8g2.setFont(FONT_DATA);
}
