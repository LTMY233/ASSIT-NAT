#include "rf433_replay.h"
#include "../core/display_mgr.h"
#include "../config.h"
#include <RCSwitch.h>

static RCSwitch rfReplay = RCSwitch();

ModuleInterface* createRf433Replay() { return new Rf433Replay(); }

void Rf433Replay::init() {
    memset(savedCodes, 0, sizeof(savedCodes));
    codeCount = 0; cursor = 0; txCount = 0;

    // Preload some common codes for demo
    static const unsigned long demoCodes[] = {
        1394005, 5592405, 349491, 315832, 730399, 217879
    };
    static const uint8_t demoBits[] = { 24, 24, 24, 24, 24, 24 };
    static const uint8_t demoProto[] = { 1, 1, 1, 3, 1, 1 };
    static const char* demoNames[] = {
        "Remote A", "Remote B", "Outlet A", "Bell A", "Curtain A", "Switch A"
    };

    for (uint8_t i = 0; i < 6 && i < RF433_REPLAY_MAX_CODES; i++) {
        savedCodes[i].value     = demoCodes[i];
        savedCodes[i].bitLength = demoBits[i];
        savedCodes[i].protocol  = demoProto[i];
        strncpy(savedCodes[i].name, demoNames[i], 15);
        savedCodes[i].name[15] = '\0';
        codeCount++;
    }
}

void Rf433Replay::enter() {
    cursor = 0; txCount = 0;
    pinMode(PIN_TX433, OUTPUT);
    digitalWrite(PIN_TX433, LOW);
    rfReplay.enableTransmit(PIN_TX433);
    rfReplay.setRepeatTransmit(5);
    displayMgr.setDirty();
}

void Rf433Replay::exit() {
    rfReplay.disableTransmit();
    pinMode(PIN_TX433, INPUT);
}

void Rf433Replay::update() {}

void Rf433Replay::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT && cursor > 0) {
        cursor--;
        displayMgr.setDirty();
    }
    if (ev == BTN_DOWN_SHORT && codeCount > 0 && cursor < codeCount - 1) {
        cursor++;
        displayMgr.setDirty();
    }
    if (ev == BTN_OK_SHORT) {
        if (codeCount > 0) {
            Rf433SavedCode* sc = &savedCodes[cursor];
            rfReplay.setProtocol(sc->protocol);
            rfReplay.send(sc->value, sc->bitLength);
            txCount++;
            displayMgr.setDirty();
        }
    }
}

void Rf433Replay::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "433 Replay");

    if (codeCount == 0) {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(10, 35, "No saved codes");
        u8g2.drawStr(0, 63, "OK back");
        return;
    }

    // Show list of codes with cursor highlight
    Rf433SavedCode* sc = &savedCodes[cursor];

    // Highlight bar for selected item
    u8g2.setFont(FONT_DATA);
    u8g2.drawBox(0, 20, OLED_WIDTH, 14);
    u8g2.setDrawColor(0);
    u8g2.drawStr(2, 31, sc->name);
    u8g2.setDrawColor(1);

    // Show code details
    char buf[32];
    snprintf(buf, sizeof(buf), "DEC:%lu", sc->value);
    u8g2.drawStr(2, 46, buf);

    snprintf(buf, sizeof(buf), "Bits:%d Proto:%d", sc->bitLength, sc->protocol);
    u8g2.drawStr(2, 56, buf);

    // Footer
    snprintf(buf, sizeof(buf), "TX:%lu [%d/%d]", txCount, cursor + 1, codeCount);
    u8g2.drawStr(0, 63, buf);
}
