#include "freq_generator.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createFreqGenerator() { return new FreqGenerator(); }

void FreqGenerator::init() {
    frequency = 1000;
    dutyCycle = 50;
    waveform = 0;
    output = false;
    editMode = 0;
}

void FreqGenerator::enter() {
    output = false;
    editMode = 0;
    pinMode(PIN_PWM_OUT, OUTPUT);
    digitalWrite(PIN_PWM_OUT, LOW);
    displayMgr.setDirty();
}

void FreqGenerator::exit() {
    analogWrite(PIN_PWM_OUT, 0);
    pinMode(PIN_PWM_OUT, INPUT);
}

void FreqGenerator::applySignal() {
    if (!output) {
        digitalWrite(PIN_PWM_OUT, LOW);
        return;
    }
    analogWriteFreq(frequency);
    analogWrite(PIN_PWM_OUT, dutyCycle * 1023 / 100);
}

void FreqGenerator::update() {}

void FreqGenerator::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            output = !output;
            applySignal();
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            switch (editMode) {
                case 0: // freq
                    if (frequency < 100) frequency += 10;
                    else if (frequency < 1000) frequency += 50;
                    else if (frequency < 10000) frequency += 100;
                    else frequency += 1000;
                    if (frequency > 40000) frequency = 40000;
                    break;
                case 1: // duty
                    if (dutyCycle < 100) dutyCycle += 5;
                    if (dutyCycle > 100) dutyCycle = 100;
                    break;
                case 2: // waveform
                    waveform = (waveform + 1) % 4;
                    break;
            }
            applySignal();
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            switch (editMode) {
                case 0: // freq
                    if (frequency <= 100) frequency -= 10;
                    else if (frequency <= 1000) frequency -= 50;
                    else if (frequency <= 10000) frequency -= 100;
                    else frequency -= 1000;
                    if (frequency < 10) frequency = 10;
                    break;
                case 1: // duty
                    if (dutyCycle > 0) dutyCycle -= 5;
                    break;
                case 2: // waveform
                    if (waveform == 0) waveform = 3;
                    else waveform--;
                    break;
            }
            applySignal();
            displayMgr.setDirty();
            break;
        case BTN_UP_DOUBLE:
            editMode = (editMode + 1) % 3;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void FreqGenerator::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Freq Generator");

    // Frequency display
    u8g2.setFont(FONT_BIG);
    char buf[20];
    if (frequency >= 1000) {
        snprintf(buf, sizeof(buf), "%.1fkHz", frequency / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%luHz", frequency);
    }
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, buf);

    // Duty cycle and waveform
    u8g2.setFont(FONT_DATA);
    static const char* waveNames[] = {"SQUARE", "SINE", "TRI", "SAW"};
    snprintf(buf, sizeof(buf), "Duty:%d%% %s", dutyCycle, waveNames[waveform]);
    u8g2.drawStr(2, 42, buf);

    // Output state
    snprintf(buf, sizeof(buf), "Out:%s", output ? "ON" : "OFF");
    u8g2.drawStr(2, 50, buf);

    // Footer
    static const char* editNames[] = {"Freq", "Duty", "Wave"};
    snprintf(buf, sizeof(buf), "Edit:%s OK:out OKx2:next",
             editNames[editMode]);
    u8g2.drawStr(0, 63, buf);
}
