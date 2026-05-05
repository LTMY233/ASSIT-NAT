#pragma once
#include "module_interface.h"
#include "../icons.h"

#define TEMP_SENSOR_MAX_CODES 10

struct TempSensorData {
    float    temperature;
    uint8_t  humidity;
    char     sensorType[16];
    uint32_t timestamp;
    bool     fresh;
};

class TempSensor : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName() const override     { return "Temp Sensor"; }
    const char* getTitle() const override    { return "433MHz Temp/Hum"; }
    uint8_t     getId() const override       { return 75; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    TempSensorData data;
    unsigned long  lastCode;
    uint32_t       captureCount;
    bool           running;

    bool tryDecode(unsigned long value, uint8_t bitLen, uint8_t proto);
};

ModuleInterface* createTempSensor();
