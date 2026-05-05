#pragma once
#include "module_interface.h"
#include "../icons.h"

class BroadcastStorm : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 5; }
    const char* getName() const override     { return "Broadcast Storm"; }
    const char* getTitle() const override    { return "Broadcast Storm"; }
    const unsigned char* getIcon() const override { return icon_storm; }
    uint8_t     getId() const override       { return 64; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    uint32_t totalPkts, broadcastPkts;
    uint8_t  broadcastPercent;
    bool     alert;
    uint32_t lastCalc;
    bool     running;

    static void onPacket(uint8_t* buf, uint16_t len);
    static BroadcastStorm* instance;
};

ModuleInterface* createBroadcastStorm();
