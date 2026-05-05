#pragma once
#include "module_interface.h"
#include "../icons.h"

class PmkidCapture : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 2; }
    const char* getName() const override     { return "PMKID Capture"; }
    const char* getTitle() const override    { return "PMKID Harvest"; }
    const unsigned char* getIcon() const override { return icon_pmkid; }
    uint8_t     getId() const override       { return 42; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    bool     running;
    bool     captured;
    uint32_t captureTime;
    uint8_t  pmkidData[128];
    uint8_t  pmkidLen;
    uint8_t  targetBssid[6];

    static void onPacket(uint8_t* buf, uint16_t len);
    static PmkidCapture* instance;
    void parseEapol(uint8_t* buf, uint16_t len);
};

ModuleInterface* createPmkidCapture();
