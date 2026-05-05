#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <ESP8266WiFi.h>

#define ROGUE_MAX_CLIENTS 20

struct RogueClient {
    uint8_t  mac[6];
    uint32_t connectTime;
};

class RogueAp : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 2; }
    const char* getName() const override     { return "Rogue AP"; }
    const char* getTitle() const override    { return "Rogue AP"; }
    const unsigned char* getIcon() const override { return icon_rogue; }
    uint8_t     getId() const override       { return 44; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    RogueClient clients[ROGUE_MAX_CLIENTS];
    uint8_t  clientCount;
    bool     running;
    uint32_t startTime;
    bool     confirmed;

    void startAp();
    void stopAp();
};

ModuleInterface* createRogueAp();
