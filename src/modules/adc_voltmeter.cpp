#include "../chinese_glyphs.h"
#include "adc_voltmeter.h"
#include "../core/display_mgr.h"
#include "../config.h"

ModuleInterface* createAdcVoltmeter() { return new AdcVoltmeter(); }

void AdcVoltmeter::init() {
    rawValue = 0; voltage = 0; lastRead = 0;
}

void AdcVoltmeter::enter() {
    pinMode(PIN_ADC_IN, INPUT);
    lastRead = 0;
    displayMgr.setDirty();
}

void AdcVoltmeter::exit() {}

void AdcVoltmeter::update() {
    if (millis() - lastRead > 500) {
        rawValue = analogRead(PIN_ADC_IN);
        voltage = rawValue * 1.0f / 1024.0f;  // ESP8266 ADC range 0-1V
        lastRead = millis();
        displayMgr.setDirty();
    }
}

void AdcVoltmeter::handleButton(ButtonEvent ev) {}

void AdcVoltmeter::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // voltage value
    u8g2.setFont(FONT_BIG);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.3fV", voltage);
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, buf);

    // mV
    u8g2.setFont(FONT_BODY);
    snprintf(buf, sizeof(buf), "%dmV", (int)(voltage * 1000));
    tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 40, buf);

    // progress bar
    uint8_t barW = (uint8_t)(voltage * OLED_WIDTH / 1.0f);
    if (barW > OLED_WIDTH) barW = OLED_WIDTH;
    u8g2.drawFrame(0, 46, OLED_WIDTH, 6);
    u8g2.drawBox(0, 46, barW, 6);

    u8g2.setFont(FONT_DATA);
    drawCN(u8g2, 2, 63, "原始:");
    snprintf(buf, sizeof(buf), "%d/1024", rawValue);
    u8g2.drawStr(2 + cnStrWidth("原始:"), 63, buf);
}
