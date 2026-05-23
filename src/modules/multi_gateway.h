#pragma once
#include "module_interface.h"
#include "../icons.h"

class MultiGateway : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 5; }
    const char* getName() const override     { return "多网关检测"; }
    const char* getTitle() const override    { return "ARP网关检测"; }
    const unsigned char* getIcon() const override { return icon_alert; }
    uint8_t     getId() const override       { return 61; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    struct GwMac {
        uint8_t mac[6];
        uint32_t firstSeen;
        uint32_t lastSeen;
    };

    GwMac    gateways[4];
    uint8_t  gwCount;
    bool     alert;
    uint32_t startTime;
    bool     running;

    static void onPacket(uint8_t* buf, uint16_t len);
    static MultiGateway* instance;
};

ModuleInterface* createMultiGateway();
