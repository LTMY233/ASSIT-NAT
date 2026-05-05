#include "gpio_control.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createGpioControl() { return new GpioControl(); }

// D0=16, D1=5, D2=4, D3=0, D4=2, D5=14, D6=12, D7=13, D8=15
const uint8_t GpioControl::pinMap[GPIO_PIN_COUNT] = {
    16, 5, 4, 0, 2, 14, 12, 13, 15
};

void GpioControl::init() {
    selectedPin = 0;
    editMode = false;
    for (uint8_t i = 0; i < GPIO_PIN_COUNT; i++) {
        pinMode_[i] = true;   // default OUTPUT
        pinState_[i] = false; // default LOW
    }
}

void GpioControl::enter() {
    // Initialize all pins as INPUT to be safe
    for (uint8_t i = 0; i < GPIO_PIN_COUNT; i++) {
        pinMode(pinMap[i], INPUT);
    }
    pinMode_[0] = true;
    pinState_[0] = false;
    pinMode(pinMap[0], OUTPUT);
    digitalWrite(pinMap[0], LOW);
    selectedPin = 0;
    editMode = false;
    displayMgr.setDirty();
}

void GpioControl::exit() {
    // Set all pins back to INPUT (safe)
    for (uint8_t i = 0; i < GPIO_PIN_COUNT; i++) {
        pinMode(pinMap[i], INPUT);
    }
}

void GpioControl::readPin(uint8_t idx) {
    pinState_[idx] = digitalRead(pinMap[idx]) == HIGH;
}

void GpioControl::writePin(uint8_t idx, bool state) {
    digitalWrite(pinMap[idx], state ? HIGH : LOW);
    pinState_[idx] = state;
}

void GpioControl::update() {}

void GpioControl::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            if (!editMode) {
                // Toggle pin mode
                pinMode_[selectedPin] = !pinMode_[selectedPin];
                if (pinMode_[selectedPin]) {
                    pinMode(pinMap[selectedPin], OUTPUT);
                    digitalWrite(pinMap[selectedPin], pinState_[selectedPin] ? HIGH : LOW);
                } else {
                    pinMode(pinMap[selectedPin], INPUT);
                }
            } else {
                // Toggle output state
                if (pinMode_[selectedPin]) {
                    pinState_[selectedPin] = !pinState_[selectedPin];
                    writePin(selectedPin, pinState_[selectedPin]);
                }
            }
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            if (!editMode) {
                if (selectedPin < GPIO_PIN_COUNT - 1) selectedPin++;
            } else {
                if (pinMode_[selectedPin]) {
                    pinState_[selectedPin] = true;
                    writePin(selectedPin, true);
                }
            }
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (!editMode) {
                if (selectedPin > 0) selectedPin--;
            } else {
                if (pinMode_[selectedPin]) {
                    pinState_[selectedPin] = false;
                    writePin(selectedPin, false);
                }
            }
            displayMgr.setDirty();
            break;
        case BTN_OK_DOUBLE:
            editMode = !editMode;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void GpioControl::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "GPIO Control");

    // Pin indicator
    u8g2.setFont(FONT_BIG);
    char buf[16];
    snprintf(buf, sizeof(buf), "D%d", selectedPin);
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 30, buf);

    // State display
    u8g2.setFont(FONT_DATA);
    const char* modeStr = pinMode_[selectedPin] ? "OUTPUT" : "INPUT";
    const char* stateStr;
    if (!pinMode_[selectedPin]) {
        readPin(selectedPin);
        stateStr = pinState_[selectedPin] ? "HIGH" : "LOW";
    } else {
        stateStr = pinState_[selectedPin] ? "HIGH" : "LOW";
    }

    snprintf(buf, sizeof(buf), "Mode:%s", modeStr);
    u8g2.drawStr(2, 44, buf);

    snprintf(buf, sizeof(buf), "State:%s", stateStr);
    u8g2.drawStr(2, 54, buf);

    // Footer
    u8g2.drawStr(0, 63, editMode ? "EDIT: UP/DN toggle" : "OK:mode OKx2:edit");
}
