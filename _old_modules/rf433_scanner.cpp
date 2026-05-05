#include "rf433_scanner.h"
#include "../core/display_mgr.h"
#include "../config.h"
#include <RCSwitch.h>

static RCSwitch rfScanner = RCSwitch();

ModuleInterface* createRf433Scanner() { return new Rf433Scanner(); }

void Rf433Scanner::init() {
    lastValue = 0; lastBitLength = 0; lastProtocol = 0;
    avgPulseUs = 0; signalCount = 0; hasData = false; running = false;
    memset(rawPulses, 0, sizeof(rawPulses));
    pulseCount = 0;
}

void Rf433Scanner::enter() {
    lastValue = 0; lastBitLength = 0; lastProtocol = 0;
    avgPulseUs = 0; signalCount = 0; hasData = false;
    memset(rawPulses, 0, sizeof(rawPulses));
    pulseCount = 0;

    pinMode(PIN_RX433, INPUT);
    rfScanner.enableReceive(PIN_RX433);
    running = true;
    displayMgr.setDirty();
}

void Rf433Scanner::exit() {
    running = false;
    rfScanner.disableReceive();
    pinMode(PIN_RX433, INPUT);
}

void Rf433Scanner::analyzePulses() {
    if (pulseCount < 2) {
        avgPulseUs = 0;
        return;
    }
    uint32_t sum = 0;
    for (uint8_t i = 0; i < pulseCount; i++) {
        sum += abs(rawPulses[i]);
    }
    avgPulseUs = sum / pulseCount;
}

const char* Rf433Scanner::guessProtocol() {
    if (!hasData || avgPulseUs == 0) return "Unknown";
    if (avgPulseUs >= 300 && avgPulseUs <= 400) return "EV1527/PT2262";
    if (avgPulseUs >= 500 && avgPulseUs <= 700) return "SC5262/HS2262";
    if (avgPulseUs >= 800 && avgPulseUs <= 1000) return "HX2262";
    if (avgPulseUs >= 150 && avgPulseUs <= 250) return "HT12E";
    if (avgPulseUs >= 100 && avgPulseUs <= 180) return "ASK/OOK short pulse";
    return "Custom OOK";
}

void Rf433Scanner::update() {
    if (!running) return;
    if (rfScanner.available()) {
        lastValue     = rfScanner.getReceivedValue();
        lastBitLength = rfScanner.getReceivedBitlength();
        lastProtocol  = rfScanner.getReceivedProtocol();
        unsigned int* raw = rfScanner.getReceivedRawdata();
        pulseCount = 0;
        if (raw) {
            for (int i = 0; i < RF433_SCAN_MAX_PULSES && raw[i] > 0; i++) {
                rawPulses[i] = raw[i];
                pulseCount++;
            }
        }
        signalCount++;
        hasData = true;
        analyzePulses();

        rfScanner.resetAvailable();
        displayMgr.setDirty();
    }
}

void Rf433Scanner::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        signalCount = 0; hasData = false;
        pulseCount = 0; avgPulseUs = 0;
        lastValue = 0; lastBitLength = 0; lastProtocol = 0;
        memset(rawPulses, 0, sizeof(rawPulses));
        displayMgr.setDirty();
    }
}

void Rf433Scanner::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "433 Scanner");

    if (!hasData) {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(10, 35, "Scanning 433MHz...");
        u8g2.drawStr(0, 63, "OK clear");
        return;
    }

    u8g2.setFont(FONT_DATA);
    char buf[32];

    // Pulse length
    snprintf(buf, sizeof(buf), "Pulse:%dus", avgPulseUs);
    u8g2.drawStr(2, 24, buf);

    // Protocol guess
    snprintf(buf, sizeof(buf), "Proto:%s", guessProtocol());
    u8g2.drawStr(2, 36, buf);

    // Signal details
    snprintf(buf, sizeof(buf), "DEC:%lu", lastValue);
    u8g2.drawStr(2, 48, buf);

    // Bits + protocol
    snprintf(buf, sizeof(buf), "Bits:%d Proto:%d", lastBitLength, lastProtocol);
    u8g2.drawStr(2, 58, buf);

    // Footer
    snprintf(buf, sizeof(buf), "Sigs:%lu", signalCount);
    u8g2.drawStr(0, 63, buf);
}
