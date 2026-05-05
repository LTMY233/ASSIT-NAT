#include "rf433_sniffer.h"
#include "../core/display_mgr.h"
#include "../config.h"
#include <RCSwitch.h>

static RCSwitch rfSniffer = RCSwitch();

ModuleInterface* createRf433Sniffer() { return new Rf433Sniffer(); }

void Rf433Sniffer::init() {
    memset(codes, 0, sizeof(codes));
    codeCount = 0; cursor = 0; totalCaptured = 0; running = false;
}

void Rf433Sniffer::enter() {
    codeCount = 0; cursor = 0; totalCaptured = 0;
    pinMode(PIN_RX433, INPUT);
    rfSniffer.enableReceive(PIN_RX433);
    running = true;
    displayMgr.setDirty();
}

void Rf433Sniffer::exit() {
    running = false;
    rfSniffer.disableReceive();
    pinMode(PIN_RX433, INPUT);
}

void Rf433Sniffer::addCode(unsigned long value, uint8_t bitLen, uint8_t proto) {
    if (codeCount >= RF433_SNIFF_MAX_CODES) {
        for (uint8_t i = 0; i < RF433_SNIFF_MAX_CODES - 1; i++) {
            codes[i] = codes[i + 1];
        }
        codeCount = RF433_SNIFF_MAX_CODES - 1;
    }
    codes[codeCount].value     = value;
    codes[codeCount].bitLength = bitLen;
    codes[codeCount].protocol  = proto;
    codes[codeCount].timestamp = millis();
    codeCount++;
    totalCaptured++;
    cursor = codeCount - 1;
}

void Rf433Sniffer::codeToBinary(unsigned long value, uint8_t bits, char* buf, size_t bufsize) {
    if (bits > 32 || bufsize < bits + 1) {
        if (bufsize > 0) buf[0] = '\0';
        return;
    }
    for (uint8_t i = 0; i < bits; i++) {
        buf[bits - 1 - i] = (value & (1UL << i)) ? '1' : '0';
    }
    buf[bits] = '\0';
}

void Rf433Sniffer::update() {
    if (!running) return;
    if (rfSniffer.available()) {
        unsigned long val = rfSniffer.getReceivedValue();
        uint8_t blen      = rfSniffer.getReceivedBitlength();
        uint8_t proto     = rfSniffer.getReceivedProtocol();
        rfSniffer.resetAvailable();

        if (val != 0 && blen > 0) {
            addCode(val, blen, proto);
            displayMgr.setDirty();
        }
    }
}

void Rf433Sniffer::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT && cursor > 0) {
        cursor--;
        displayMgr.setDirty();
    }
    if (ev == BTN_DOWN_SHORT && codeCount > 0 && cursor < codeCount - 1) {
        cursor++;
        displayMgr.setDirty();
    }
    if (ev == BTN_OK_SHORT) {
        codeCount = 0; cursor = 0; totalCaptured = 0;
        memset(codes, 0, sizeof(codes));
        displayMgr.setDirty();
    }
}

void Rf433Sniffer::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "433 Sniffer");

    if (codeCount == 0) {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(10, 35, "Wait 433MHz signal...");
        u8g2.drawStr(0, 63, "OK clear");
        return;
    }

    Rf433Code* c = &codes[cursor];

    u8g2.setFont(FONT_DATA);
    // Decimal value
    char buf[32];
    snprintf(buf, sizeof(buf), "DEC:%lu", c->value);
    u8g2.drawStr(2, 24, buf);

    // Binary value
    char binBuf[40];
    codeToBinary(c->value, c->bitLength, binBuf, sizeof(binBuf));
    snprintf(buf, sizeof(buf), "BIN:%s", binBuf);
    u8g2.drawStr(2, 38, buf);

    // Protocol and bit length
    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "Proto:%d Bits:%d", c->protocol, c->bitLength);
    u8g2.drawStr(2, 52, buf);

    // Footer: count + cursor
    snprintf(buf, sizeof(buf), "#%lu [%d/%d] UP/DN", totalCaptured, cursor + 1, codeCount);
    u8g2.drawStr(0, 63, buf);
}
