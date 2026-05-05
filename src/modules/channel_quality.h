#pragma once
#include "module_interface.h"
#include "../icons.h"

class ChannelQuality : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 1; }
    const char* getName() const override     { return "Channel Quality"; }
    const char* getTitle() const override    { return "Channel Quality"; }
    const unsigned char* getIcon() const override { return icon_quality; }
    uint8_t     getId() const override       { return 23; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    struct ChQuality {
        uint8_t channel;
        int8_t  score;       // 0-100
        uint8_t apCount;
        int8_t  avgRssi;
    };

    ChQuality channels[13];
    uint8_t  bestChannel;
    bool     hasData;
    uint32_t lastScan;

    void doScan();
    void calcScores();
};

ModuleInterface* createChannelQuality();
