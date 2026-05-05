#include "temp_sensor.h"
#include "../core/display_mgr.h"
#include "../config.h"
#include <RCSwitch.h>

static RCSwitch rfTemp = RCSwitch();

ModuleInterface* createTempSensor() { return new TempSensor(); }

void TempSensor::init() {
    memset(&data, 0, sizeof(data));
    data.temperature = 0.0f;
    data.humidity    = 0;
    strcpy(data.sensorType, "Unknown");
    data.timestamp = 0;
    data.fresh     = false;
    lastCode       = 0;
    captureCount   = 0;
    running        = false;
}

void TempSensor::enter() {
    memset(&data, 0, sizeof(data));
    strcpy(data.sensorType, "Unknown");
    lastCode     = 0;
    captureCount = 0;

    pinMode(PIN_RX433, INPUT);
    rfTemp.enableReceive(PIN_RX433);
    running = true;
    displayMgr.setDirty();
}

void TempSensor::exit() {
    running = false;
    rfTemp.disableReceive();
    pinMode(PIN_RX433, INPUT);
}

bool TempSensor::tryDecode(unsigned long value, uint8_t bitLen, uint8_t proto) {
    // Common 433MHz temperature sensor protocols
    // Many sensors use 24-bit codes where temp/humidity are encoded

    if (bitLen < 20 || bitLen > 32) return false;
    if (value == lastCode) return false;  // Dedup

    lastCode = value;
    captureCount++;

    // Try to parse as common sensor format
    // Format 1: 24-bit, upper 12 = temperature * 10, lower 12 = humidity * 10 + checksum
    uint8_t tries = 0;

    // Try protocol 1: typical EV1527-style sensor
    // Bits: [8-bit ID][8-bit temp/2][8-bit hum]
    if (bitLen >= 24) {
        uint8_t tempRaw = (value >> 16) & 0xFF;
        uint8_t humRaw  = (value >> 8) & 0xFF;

        // Check if values look reasonable
        float t = (tempRaw - 50) * 0.5f + 10.0f;  // Try offset scaling
        if (t >= -20.0f && t <= 60.0f && humRaw <= 100) {
            data.temperature = t;
            data.humidity    = humRaw;
            strcpy(data.sensorType, "EV1527-OOK");
            data.timestamp = millis();
            data.fresh     = true;
            return true;
        }
    }

    // Try protocol 2: temperature in upper 12 bits * 0.1
    if (bitLen >= 22) {
        uint16_t tempRaw = (value >> 10) & 0xFFF;
        uint8_t  humRaw  = (value >> 4) & 0x3F;

        float t = (tempRaw - 400) * 0.1f;
        if (t >= -20.0f && t <= 60.0f && humRaw <= 100) {
            data.temperature = t;
            data.humidity    = humRaw;
            strcpy(data.sensorType, "OOK-12bit");
            data.timestamp = millis();
            data.fresh     = true;
            return true;
        }
    }

    // Try protocol 3: simple 24-bit, temp = (value >> 12) & 0xFFF * 0.1
    if (bitLen >= 24) {
        int16_t tempSigned = (int16_t)((value >> 12) & 0xFFF);
        if (tempSigned & 0x800) tempSigned |= 0xF000;  // Sign extend

        float t = tempSigned * 0.1f;
        uint8_t h = (value >> 4) & 0xFF;

        if (t >= -20.0f && t <= 60.0f && h <= 100) {
            data.temperature = t;
            data.humidity    = h;
            strcpy(data.sensorType, "Lacrosse/TH");
            data.timestamp = millis();
            data.fresh     = true;
            return true;
        }
    }

    // Fallback: store raw value but mark as unknown
    data.temperature = 0.0f;
    data.humidity    = 0;
    strcpy(data.sensorType, "UnkOOK");
    data.timestamp = millis();
    data.fresh     = true;
    return false;
}

void TempSensor::update() {
    if (!running) return;
    if (rfTemp.available()) {
        unsigned long val  = rfTemp.getReceivedValue();
        uint8_t blen       = rfTemp.getReceivedBitlength();
        uint8_t proto      = rfTemp.getReceivedProtocol();
        rfTemp.resetAvailable();

        if (val != 0 && blen > 0) {
            tryDecode(val, blen, proto);
            displayMgr.setDirty();
        }
    }
}

void TempSensor::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        memset(&data, 0, sizeof(data));
        strcpy(data.sensorType, "Unknown");
        lastCode     = 0;
        captureCount = 0;
        displayMgr.setDirty();
    }
}

void TempSensor::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Temp Sensor");

    if (!data.fresh) {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(5, 35, "Wait sensor data...");
        u8g2.drawStr(0, 63, "OK clear");
        return;
    }

    u8g2.setFont(FONT_DATA);
    char buf[32];

    // Temperature (large)
    u8g2.setFont(FONT_BIG);
    snprintf(buf, sizeof(buf), "%.1f C", data.temperature);
    uint8_t tw = u8g2.getStrWidth(buf);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, buf);

    // Humidity
    u8g2.setFont(FONT_DATA);
    snprintf(buf, sizeof(buf), "Humidity:%d%%", data.humidity);
    u8g2.drawStr(2, 42, buf);

    // Sensor type
    snprintf(buf, sizeof(buf), "Type:%s", data.sensorType);
    u8g2.drawStr(2, 52, buf);

    // Footer: last update + count
    uint32_t elapsed = (millis() - data.timestamp) / 1000;
    snprintf(buf, sizeof(buf), "T-%lus #%lu", elapsed, captureCount);
    u8g2.drawStr(0, 63, buf);
}
