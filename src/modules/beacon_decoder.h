#pragma once
#include "module_interface.h"
#include "../icons.h"

struct BeaconInfo {
    uint8_t  bssid[6];
    char     ssid[33];
    uint8_t  channel;
    int8_t   rssi;
    uint8_t  encType;
    char     country[4];
    uint8_t  supportedRates[8];
    uint8_t  rateCount;
    bool     htCapable;
    bool     wpsEnabled;
    uint8_t  vendorSpecific[32];
    uint8_t  vendorLen;
};

class BeaconDecoder : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 1; }
    const char* getName() const override     { return "Beacon Decoder"; }
    const char* getTitle() const override    { return "Beacon Decoder"; }
    const unsigned char* getIcon() const override { return icon_beacon; }
    uint8_t     getId() const override       { return 20; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    BeaconInfo beacon;
    bool    hasData;
    uint8_t cursor;  // scroll position
    bool    running;

    static void onPacket(uint8_t* buf, uint16_t len);
    static BeaconDecoder* instance;
    bool parseBeacon(uint8_t* buf, uint16_t len);
};

ModuleInterface* createBeaconDecoder();
