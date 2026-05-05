#pragma once
#include "module_interface.h"
#include "../icons.h"

#define MONITOR_HISTORY 64

class SignalMonitor : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 1; }
    const char* getName() const override     { return "Signal Monitor"; }
    const char* getTitle() const override    { return "Signal Monitor"; }
    const unsigned char* getIcon() const override { return icon_sniffer; }
    uint8_t     getId() const override       { return 27; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    enum { SIGMON_IDLE, SIGMON_SCANNING, SIGMON_TRACKING } state;
    char     apSsid[33];
    uint8_t  apBssid[6];
    int32_t  apChannel;
    int8_t   currentRssi;
    int8_t   prevRssi;
    int8_t   rssiHistory[MONITOR_HISTORY];
    uint8_t  histIdx;
    uint32_t lastUpdate;
};

ModuleInterface* createSignalMonitor();
