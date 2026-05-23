#include "../chinese_glyphs.h"
#include "led_test.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createLedTest() { return new LedTest(); }

void LedTest::init() {
    currentPattern = LED_BLINK;
    running = false;
    ledState = false;
    lastToggle = 0;
    stepIndex = 0;
    // SOS: short=150ms, long=450ms, gap between=150ms
    sosTimings[0] = 150;  // S dot
    sosTimings[1] = 150;  // S dot
    sosTimings[2] = 150;  // S dot
    sosTimings[3] = 450;  // O dash
    sosTimings[4] = 450;  // O dash
    sosTimings[5] = 450;  // O dash
}

void LedTest::enter() {
    currentPattern = LED_BLINK;
    running = false;
    ledState = false;
    lastToggle = 0;
    stepIndex = 0;
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);  // LED off (active low)
    displayMgr.setDirty();
}

void LedTest::exit() {
    digitalWrite(LED_BUILTIN, HIGH);  // LED off
    pinMode(LED_BUILTIN, INPUT);
}

const char* LedTest::getPatternName(uint8_t p) {
    switch (p) {
        case LED_BLINK:     return "Blink";
        case LED_SOS:       return "SOS";
        case LED_FAST:      return "快";
        case LED_SLOW:      return "Slow";
        case LED_HEARTBEAT: return "心跳";
        default: return "Unknown";
    }
}

uint32_t LedTest::getPatternInterval(uint8_t p) {
    switch (p) {
        case LED_BLINK:     return 500;
        case LED_FAST:      return 100;
        case LED_SLOW:      return 1000;
        case LED_HEARTBEAT: return 300;
        case LED_SOS:       return 0;  // handled separately
        default: return 500;
    }
}

void LedTest::updateLed() {
    uint32_t now = millis();

    if (currentPattern == LED_SOS) {
        // SOS uses stepIndex to cycle through dots and dashes
        if (now - lastToggle >= sosTimings[stepIndex] + 300) {
            // Gap between symbols
            digitalWrite(LED_BUILTIN, HIGH);  // off
            ledState = false;
            if (now - lastToggle >= sosTimings[stepIndex] + 450) {
                stepIndex = (stepIndex + 1) % 6;
                lastToggle = now;
            }
        } else if (now - lastToggle >= sosTimings[stepIndex]) {
            // Toggle for next symbol element
            ledState = !ledState;
            digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
            if (ledState) lastToggle = now;  // start timing the ON period
        }
        return;
    }

    if (currentPattern == LED_HEARTBEAT) {
        // Heartbeat: double pulse pattern
        uint32_t elapsed = now - lastToggle;
        if (ledState) {
            // ON phases: short 100ms, then after 200ms another short 100ms, then 700ms off
            uint32_t cyclePos = elapsed % 1100;
            if (cyclePos < 100 || (cyclePos >= 300 && cyclePos < 400)) {
                digitalWrite(LED_BUILTIN, LOW);
            } else {
                digitalWrite(LED_BUILTIN, HIGH);
            }
        }
        return;
    }

    // Simple blink patterns
    uint32_t interval = getPatternInterval(currentPattern);
    if (now - lastToggle >= interval) {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
        lastToggle = now;
    }
}

void LedTest::update() {
    if (running) {
        updateLed();
        // Only set dirty periodically to avoid excessive rendering
        static uint32_t lastDraw = 0;
        if (millis() - lastDraw > 200) {
            displayMgr.setDirty();
            lastDraw = millis();
        }
    }
}

void LedTest::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_UP_SHORT:
            if (!running) {
                if (currentPattern > 0) currentPattern--;
                else currentPattern = LED_PAT_COUNT - 1;
                displayMgr.setDirty();
            }
            break;
        case BTN_DOWN_SHORT:
            if (!running) {
                if (currentPattern < LED_PAT_COUNT - 1) currentPattern++;
                else currentPattern = 0;
                displayMgr.setDirty();
            }
            break;
        case BTN_OK_SHORT:
            running = !running;
            if (!running) {
                digitalWrite(LED_BUILTIN, HIGH);  // off
                ledState = false;
            } else {
                lastToggle = millis();
                ledState = false;
                stepIndex = 0;
            }
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void LedTest::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    u8g2.setFont(FONT_BODY);
    char buf[32];

    // Pattern name
    snprintf(buf, sizeof(buf), "模式: %s", getPatternName(currentPattern));
    drawCN(u8g2, 2, 24, buf);

    // Status
    snprintf(buf, sizeof(buf), "态: %s",
             running ? (ledState ? "开" : "关") : "已停止");
    drawCN(u8g2, 2, 33, buf);

    // Interval for non-SOS patterns
    if (currentPattern != LED_SOS) {
        snprintf(buf, sizeof(buf), "时:%lums",
                 (unsigned long)getPatternInterval(currentPattern));
        drawCN(u8g2, 2, 42, buf);
    } else {
        u8g2.drawStr(2, 42, "SOS: ...---...");
    }

    // LED visual indicator
    uint8_t ledX = OLED_WIDTH - 20;
    uint8_t ledY = 30;
    uint8_t ledR = 6;
    if (running && ledState) {
        u8g2.drawDisc(ledX, ledY, ledR);
    } else {
        u8g2.drawCircle(ledX, ledY, ledR);
    }

    u8g2.setFont(FONT_DATA);
    if (running) {
    } else {
    }
}
