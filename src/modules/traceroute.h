#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <ESP8266WiFi.h>

#define TR_MAX_HOPS 8

struct TraceHop {
    uint8_t   ttl;
    IPAddress ip;
    int16_t   rtt;     // -1 = timeout (*)
};

class Traceroute : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "Traceroute"; }
    const char* getTitle() const override    { return "Traceroute"; }
    const unsigned char* getIcon() const override { return icon_traceroute; }
    uint8_t     getId() const override       { return 6; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    TraceHop hops[TR_MAX_HOPS];
    uint8_t  hopCount;
    uint8_t  currentTtl;
    bool     tracing;
    uint32_t lastProbe;
    IPAddress targetIP;

    void startTrace(IPAddress target);
    void sendProbe(uint8_t ttl);
    bool checkReply();
};

ModuleInterface* createTraceroute();
