#include "../chinese_glyphs.h"
#include "battery_adc.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createBatteryAdc() { return new BatteryAdc(); }

void BatteryAdc::init() {
    rawValue = 0; voltage_meas = 0; batteryVolts = 0;
    batteryPercent = 0; lastRead = 0;
    batMin = 3.0f; batMax = 4.2f;
}

void BatteryAdc::enter() {
    pinMode(PIN_ADC_IN, INPUT);
    lastRead = 0;
    readBattery();
    displayMgr.setDirty();
}

void BatteryAdc::exit() {}

void BatteryAdc::readBattery() {
    rawValue = analogRead(PIN_ADC_IN);
    voltage_meas = rawValue * 1.0f / 1024.0f;  // ESP8266 ADC 0-1V
    // Assume 4:1 voltage divider (e.g., 100k/27k) giving ~0.84V at 4.2V battery
    // Actual ratio = (R1+R2)/R2 — user may need to adjust this factor
    batteryVolts = voltage_meas * 5.0f;  // 5x multiplier for common divider

    // Calculate percentage
    if (batteryVolts >= batMax) batteryPercent = 100;
    else if (batteryVolts <= batMin) batteryPercent = 0;
    else batteryPercent = (uint8_t)((batteryVolts - batMin) * 100.0f / (batMax - batMin));
    if (batteryPercent > 100) batteryPercent = 100;
}

void BatteryAdc::update() {
    if (millis() - lastRead > 500) {
        readBattery();
        lastRead = millis();
        displayMgr.setDirty();
    }
}

void BatteryAdc::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT || ev == BTN_UP_SHORT || ev == BTN_DOWN_SHORT) {
        readBattery();
        lastRead = millis();
        displayMgr.setDirty();
    }
}

void BatteryAdc::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // Voltage reading
    u8g2.setFont(FONT_BIG);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2fV", batteryVolts);
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 30, buf);

    // Percentage
    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "%d%%", batteryPercent);
    tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 42, buf);

    // Bar graph for percentage
    uint8_t barY = 50;
    uint8_t barH = 6;
    uint8_t barW = OLED_WIDTH - 4;
    u8g2.drawFrame(2, barY, barW, barH);
    uint8_t fillW = (uint8_t)((uint32_t)batteryPercent * barW / 100);
    u8g2.drawBox(2, barY, fillW, barH);

    // Raw ADC footer
    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "Raw:%d/1024 %.3fV",
             rawValue, voltage_meas);
    u8g2.drawStr(0, 63, buf);
}
