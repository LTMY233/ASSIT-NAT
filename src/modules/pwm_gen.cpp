#include "../chinese_glyphs.h"
#include "pwm_gen.h"
#include "../core/display_mgr.h"
#include "../config.h"

ModuleInterface* createPwmGen() { return new PwmGen(); }

void PwmGen::init() {
    frequency = 1000; dutyCycle = 50;
    output = false; editMode = 0;
}

void PwmGen::enter() {
    output = false; editMode = 0;
    pinMode(PIN_PWM_OUT, OUTPUT);
    digitalWrite(PIN_PWM_OUT, LOW);
    displayMgr.setDirty();
}

void PwmGen::exit() {
    analogWrite(PIN_PWM_OUT, 0);
    pinMode(PIN_PWM_OUT, INPUT);
}

void PwmGen::applyPwm() {
    if (!output) {
        digitalWrite(PIN_PWM_OUT, LOW);
        return;
    }
    analogWriteFreq(frequency);
    analogWrite(PIN_PWM_OUT, dutyCycle * 1023 / 100);
}

void PwmGen::update() {}

void PwmGen::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            output = !output;
            applyPwm();
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            if (editMode == 0) {
                if (frequency < 1000) frequency += 10;
                else if (frequency < 10000) frequency += 100;
                else frequency += 1000;
                if (frequency > 40000) frequency = 40000;
            } else {
                if (dutyCycle < 100) dutyCycle += 5;
            }
            applyPwm();
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (editMode == 0) {
                if (frequency <= 1000) frequency -= 10;
                else if (frequency <= 10000) frequency -= 100;
                else frequency -= 1000;
                if (frequency < 10) frequency = 10;
            } else {
                if (dutyCycle > 0) dutyCycle -= 5;
            }
            applyPwm();
            displayMgr.setDirty();
            break;
        case BTN_UP_DOUBLE:
            editMode = (editMode + 1) % 2;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void PwmGen::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    u8g2.setFont(FONT_BIG);
    char buf[20];
    if (frequency >= 1000) {
        snprintf(buf, sizeof(buf), "%.1fkHz", frequency / 1000.0f);
    } else {
        snprintf(buf, sizeof(buf), "%dHz", frequency);
    }
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, buf);

    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "Duty:%d%% ", dutyCycle);
    u8g2.drawStr(2, 40, buf);
    drawCN(u8g2, 2 + u8g2.getStrWidth(buf), 41, output ? "开" : "关");

    drawCN(u8g2, 0, 46, "编辑:");
    uint8_t epos = cnStrWidth("编辑:");
    drawCN(u8g2, epos, 46, editMode == 0 ? "频率" : "Duty");
    u8g2.drawStr(epos + cnStrWidth(editMode == 0 ? "频率" : "Duty"), 46,
                 " UP/DN+/- dbl=sw");

}
