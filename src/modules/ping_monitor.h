#pragma once
#include "module_interface.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

class PingMonitor : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "Ping Monitor"; }
    const char* getTitle() const override    { return "Ping Monitor"; }
    uint8_t     getId() const override       { return 13; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    IPAddress gateway;
    int16_t  currentRtt;
    int16_t  minRtt, maxRtt;
    int32_t  sumRtt;
    uint16_t pingCount, successCount;
    uint8_t  lossPercent;
    uint16_t lastRtt;
    int8_t   rttHistory[42];  // for bar chart, -1=no data

    uint32_t lastPing;
    bool     pingSent;
    uint32_t pingStart;
    uint16_t pingSeq;
    uint8_t  histIdx;

    void sendPing();
    bool checkReply();
    uint16_t icmpChecksum(uint16_t* buf, int len);
};

ModuleInterface* createPingMonitor();
