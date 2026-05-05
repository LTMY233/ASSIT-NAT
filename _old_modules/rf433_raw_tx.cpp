#include "rf433_raw_tx.h"
#include "../core/display_mgr.h"
#include "../config.h"
#include <RCSwitch.h>

static RCSwitch rfRawTx = RCSwitch();

ModuleInterface* createRf433RawTx() { return new Rf433RawTx(); }

void Rf433RawTx::init() {
    code         = 0x556688;
    bitLength    = 24;
    protocol     = 1;
    pulseLengthUs = 320;
    editPos      = 0;
    editMode     = 0;
    txCount      = 0;
    sent         = false;
}

void Rf433RawTx::enter() {
    sent = false; txCount = 0;
    pinMode(PIN_TX433, OUTPUT);
    digitalWrite(PIN_TX433, LOW);
    rfRawTx.enableTransmit(PIN_TX433);
    rfRawTx.setRepeatTransmit(4);
    displayMgr.setDirty();
}

void Rf433RawTx::exit() {
    rfRawTx.disableTransmit();
    pinMode(PIN_TX433, INPUT);
}

void Rf433RawTx::update() {
    if (sent) {
        sent = false;
        displayMgr.setDirty();
    }
}

void Rf433RawTx::bitsToString(char* buf, size_t bufsize) {
    if (bitLength > RF433_RAW_MAX_BITS || bufsize < bitLength + 5) {
        if (bufsize > 0) buf[0] = '\0';
        return;
    }
    uint8_t outPos = 0;
    for (uint8_t i = 0; i < bitLength; i++) {
        if (i > 0 && (i % 8) == 0) {
            buf[outPos++] = ' ';
        }
        buf[outPos++] = (code & (1UL << (bitLength - 1 - i))) ? '1' : '0';
    }
    buf[outPos] = '\0';
}

void Rf433RawTx::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_UP_SHORT:
            if (editMode == 0) {
                // Toggle bit at editPos
                code ^= (1UL << (bitLength - 1 - editPos));
            } else if (editMode == 1) {
                if (protocol < 7) protocol++;
            } else {
                pulseLengthUs += 10;
                if (pulseLengthUs > 1000) pulseLengthUs = 1000;
            }
            displayMgr.setDirty();
            break;

        case BTN_DOWN_SHORT:
            if (editMode == 0) {
                code ^= (1UL << (bitLength - 1 - editPos));
            } else if (editMode == 1) {
                if (protocol > 1) protocol--;
            } else {
                if (pulseLengthUs > 50) pulseLengthUs -= 10;
            }
            displayMgr.setDirty();
            break;

        case BTN_OK_SHORT:
            // Send
            rfRawTx.setProtocol(protocol);
            rfRawTx.setPulseLength(pulseLengthUs);
            rfRawTx.send(code, bitLength);
            txCount++; sent = true;
            displayMgr.setDirty();
            break;

        case BTN_OK_DOUBLE:
            // Cycle edit mode
            editMode = (editMode + 1) % 3;
            displayMgr.setDirty();
            break;

        case BTN_UP_DOUBLE:
            // Move edit cursor
            if (editPos > 0) { editPos--; displayMgr.setDirty(); }
            break;

        case BTN_DOWN_DOUBLE:
            // Move edit cursor
            if (editPos + 1 < bitLength) { editPos++; displayMgr.setDirty(); }
            break;

        default: break;
    }
}

void Rf433RawTx::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "433 Raw TX");

    char buf[48];

    // Binary code with edit position indicator
    char binStr[80];
    bitsToString(binStr, sizeof(binStr));
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 24, binStr);

    // Edit position marker
    u8g2.setFont(FONT_DATA);
    uint8_t markerX = editPos * 7;
    uint8_t groupsBefore = editPos / 8;
    markerX += groupsBefore * 7;  // space between groups
    u8g2.drawStr(markerX, 20, "^");

    // Code info
    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "DEC:%lu", code);
    u8g2.drawStr(2, 38, buf);

    // Protocol, pulse, bit length
    snprintf(buf, sizeof(buf), "Proto:%d Pul:%dus Bit:%d",
             protocol, pulseLengthUs, bitLength);
    u8g2.drawStr(2, 48, buf);

    // Edit mode indicator
    const char* modeStr;
    switch (editMode) {
        case 0: modeStr = "EDIT:BIT"; break;
        case 1: modeStr = "EDIT:PROTO"; break;
        case 2: modeStr = "EDIT:PULSE"; break;
        default: modeStr = "?";
    }
    u8g2.drawStr(2, 58, buf);

    // Footer
    snprintf(buf, sizeof(buf), "TX:%lu %s", txCount, modeStr);
    u8g2.drawStr(0, 63, buf);
}
