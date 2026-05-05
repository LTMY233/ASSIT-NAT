#pragma once
#include "module_interface.h"
#include "../icons.h"

class NetStats : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "Net Stats"; }
    const char* getTitle() const override    { return "Net Throughput"; }
    const unsigned char* getIcon() const override { return icon_net_stats; }
    uint8_t     getId() const override       { return 7; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    uint32_t lastCheck;
    uint32_t prevRxBytes, prevTxBytes;
    uint32_t rxRate, txRate;       // bytes/sec
    uint32_t rxTotal, txTotal;
    uint32_t rxPackets, txPackets;
    uint8_t  rxHistory[60];   // rate history (normalized)
    uint8_t  historyIdx;

    void readNetInfo();
};

ModuleInterface* createNetStats();
