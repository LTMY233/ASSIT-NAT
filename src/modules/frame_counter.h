#pragma once
#include "module_interface.h"
#include "../icons.h"

class FrameCounter : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName() const override     { return "帧计数器"; }
    const char* getTitle() const override    { return "帧速率计数器"; }
    const unsigned char* getIcon() const override { return icon_frame; }
    uint8_t     getId() const override       { return 21; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    // Frame type counts
    uint32_t mgmtCount, ctrlCount, dataCount;
    uint32_t beaconCount, probeReqCount, probeRespCount;
    uint32_t ackCount, rtsCount, ctsCount;
    uint32_t dataFrames, nullFrames;

    uint32_t mgmtRate, ctrlRate, dataRate;  // per second
    uint32_t totalPkts;
    uint32_t lastCalc;
    bool     running;

    static void onPacket(uint8_t* buf, uint16_t len);
    static FrameCounter* instance;
    void calcRates();
};

ModuleInterface* createFrameCounter();
