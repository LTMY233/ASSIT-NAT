#include "rf433_devdb.h"
#include "../core/display_mgr.h"
#include "../config.h"
#include <RCSwitch.h>

static RCSwitch rfDb = RCSwitch();

ModuleInterface* createRf433DevDb() { return new Rf433DevDb(); }

void Rf433DevDb::loadDeviceDB() {
    deviceCount = 0;
    uint8_t idx = 0;

    auto add = [&](const char* name, unsigned long code, uint8_t bits, uint8_t proto) {
        if (idx >= RF433_DB_MAX_DEVICES) return;
        strncpy(devices[idx].name, name, 19);
        devices[idx].name[19] = '\0';
        devices[idx].code      = code;
        devices[idx].bitLength = bits;
        devices[idx].protocol  = proto;
        idx++;
        deviceCount = idx;
    };

    add("EV1527 Remote A",    1394005,  24, 1);
    add("PT2262 Remote B",    5592405,  24, 1);
    add("SC5262 Outlet 1",    349491,   24, 2);
    add("HS2262 Doorbell",     315832,   24, 3);
    add("HX2262 Curtain",     730399,   24, 1);
    add("HT12E  Alarm",   217879,   24, 1);
    add("EV1527 Learn 1",  8612345,  24, 1);
    add("PT2262 Learn 2",  12345678, 24, 1);
    add("SC5262 Roller A",  5555555,  24, 2);
    add("HS2262 Fan", 11112222, 24, 2);
    add("EV1527 Garage 1",  33338888, 24, 1);
    add("PT2262 Garage 2",  77774444, 24, 1);
}

void Rf433DevDb::init() {
    memset(devices, 0, sizeof(devices));
    deviceCount = 0; cursor = 0; detailView = false; txCount = 0;
    loadDeviceDB();
}

void Rf433DevDb::enter() {
    cursor = 0; detailView = false; txCount = 0;
    pinMode(PIN_TX433, OUTPUT);
    digitalWrite(PIN_TX433, LOW);
    rfDb.enableTransmit(PIN_TX433);
    rfDb.setRepeatTransmit(5);
    displayMgr.setDirty();
}

void Rf433DevDb::exit() {
    rfDb.disableTransmit();
    pinMode(PIN_TX433, INPUT);
}

void Rf433DevDb::update() {}

void Rf433DevDb::codeToBinary(unsigned long value, uint8_t bits, char* buf, size_t bufsize) {
    if (bits > 32 || bufsize < bits + 1) {
        if (bufsize > 0) buf[0] = '\0';
        return;
    }
    for (uint8_t i = 0; i < bits; i++) {
        buf[bits - 1 - i] = (value & (1UL << i)) ? '1' : '0';
    }
    buf[bits] = '\0';
}

void Rf433DevDb::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT) {
        if (detailView) {
            detailView = false;
        } else if (cursor > 0) {
            cursor--;
        }
        displayMgr.setDirty();
    }
    if (ev == BTN_DOWN_SHORT) {
        if (detailView) {
            detailView = false;
        } else if (deviceCount > 0 && cursor < deviceCount - 1) {
            cursor++;
        }
        displayMgr.setDirty();
    }
    if (ev == BTN_OK_SHORT) {
        if (detailView && deviceCount > 0) {
            // Send the code
            Rf433Device* dev = &devices[cursor];
            rfDb.setProtocol(dev->protocol);
            rfDb.send(dev->code, dev->bitLength);
            txCount++;
        } else {
            detailView = true;
        }
        displayMgr.setDirty();
    }
}

void Rf433DevDb::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "433 Device DB");

    if (deviceCount == 0) {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(10, 35, "DB empty");
        u8g2.drawStr(0, 63, "OK back");
        return;
    }

    Rf433Device* dev = &devices[cursor];
    char buf[40];

    if (!detailView) {
        // List view: show highlighted item + prev/next hints
        u8g2.setFont(FONT_DATA);

        // Show previous item (dim)
        if (cursor > 0) {
            snprintf(buf, sizeof(buf), " %s", devices[cursor - 1].name);
            u8g2.drawStr(4, 24, buf);
        }

        // Highlighted current item
        u8g2.drawBox(0, 28, OLED_WIDTH, 14);
        u8g2.setDrawColor(0);
        u8g2.drawStr(2, 39, dev->name);
        u8g2.setDrawColor(1);

        // Show next item (dim)
        if (cursor + 1 < deviceCount) {
            snprintf(buf, sizeof(buf), " %s", devices[cursor + 1].name);
            u8g2.drawStr(4, 52, buf);
        }

        // Footer
        snprintf(buf, sizeof(buf), "[%d/%d] OK=detail", cursor + 1, deviceCount);
        u8g2.drawStr(0, 63, buf);
    } else {
        // Detail view
        u8g2.setFont(FONT_DATA);

        // Device name
        snprintf(buf, sizeof(buf), "Dev:%s", dev->name);
        u8g2.drawStr(2, 24, buf);

        // Decimal code
        snprintf(buf, sizeof(buf), "DEC:%lu", dev->code);
        u8g2.drawStr(2, 34, buf);

        // Binary code (first 24 bits)
        char binBuf[40];
        codeToBinary(dev->code, dev->bitLength, binBuf, sizeof(binBuf));
        snprintf(buf, sizeof(buf), "BIN:%s", binBuf);
        u8g2.drawStr(2, 44, buf);

        // Protocol and bits
        snprintf(buf, sizeof(buf), "Proto:%d Bits:%d", dev->protocol, dev->bitLength);
        u8g2.drawStr(2, 54, buf);

        // Footer
        snprintf(buf, sizeof(buf), "TX:%lu OK=send UP=back", txCount);
        u8g2.drawStr(0, 63, buf);
    }
}
