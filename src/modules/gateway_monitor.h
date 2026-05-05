#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

class GatewayMonitor : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "Gateway Monitor"; }
    const char* getTitle() const override    { return "Gateway Monitor"; }
    const unsigned char* getIcon() const override { return icon_gateway; }
    uint8_t     getId() const override       { return 4; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    IPAddress gateway;
    int16_t  currentRtt;
    int16_t  minRtt, maxRtt, avgRtt;
    uint32_t totalPings, successPings;
    uint8_t  lossPercent;
    uint16_t jitter;
    int16_t  lastRtt;

    uint32_t lastPing;
    bool     pingSent;
    uint32_t pingStart;
    uint16_t pingSeq;

    void sendPing();
    bool checkPingReply();
    uint16_t checksum(uint16_t* buf, int len);
};

ModuleInterface* createGatewayMonitor();
