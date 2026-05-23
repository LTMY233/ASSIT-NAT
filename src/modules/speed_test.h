#pragma once
#include "module_interface.h"
#include "../icons.h"

#define SPEED_TEST_URL  "http://speedtest.tele2.net/1MB.zip"

class SpeedTest : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 5; }
    const char* getName() const override     { return "网速测试"; }
    const char* getTitle() const override    { return "网速测试"; }
    const unsigned char* getIcon() const override { return icon_net_stats; }
    uint8_t     getId() const override       { return 68; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    bool     running;
    bool     done;
    float    speedKBps;
    uint32_t bytesDownloaded;
    uint32_t startTime;
    uint32_t elapsedMs;
    bool     wifiConnected;

    void startTest();
    void stopTest();
    void connectWiFi();
};

ModuleInterface* createSpeedTest();
