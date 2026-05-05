#pragma once
#include "module_interface.h"
#include "../icons.h"

#define KARMA_MAX_PROBES 50

struct KarmaProbe {
    uint8_t  clientMac[6];
    char     ssid[33];
    int8_t   rssi;
    uint32_t timestamp;
};

class KarmaAttack : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 2; }
    const char* getName() const override     { return "Karma Attack"; }
    const char* getTitle() const override    { return "Karma Attack"; }
    const unsigned char* getIcon() const override { return icon_karma; }
    uint8_t     getId() const override       { return 45; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    KarmaProbe probes[KARMA_MAX_PROBES];
    uint16_t probeCount;
    bool     running;
    bool     confirmed;
    uint32_t startTime;
    uint32_t lastCleanup;

    static void onPacket(uint8_t* buf, uint16_t len);
    static KarmaAttack* instance;
    void sendProbeResponse(const uint8_t* clientMac, const char* ssid);
};

ModuleInterface* createKarmaAttack();
