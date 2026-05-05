#pragma once
#include "module_interface.h"
#include "../icons.h"

#define PROBE_MAX_DEVICES 40

struct ProbeDevice {
    uint8_t  mac[6];
    int8_t   rssi;
    char     lastSsid[33];
    uint32_t lastSeen;
};

class ProbeSniffer : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "Probe Sniffer"; }
    const char* getTitle() const override    { return "Probe Sniffer"; }
    const unsigned char* getIcon() const override { return icon_sniffer; }
    uint8_t     getId() const override       { return 2; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    ProbeDevice devices[PROBE_MAX_DEVICES];
    uint8_t  deviceCount;
    uint32_t lastCleanup;
    uint8_t  cursor;
    bool     running;

    static void onPacket(uint8_t* buf, uint16_t len);
    static ProbeSniffer* instance;
};

ModuleInterface* createProbeSniffer();
