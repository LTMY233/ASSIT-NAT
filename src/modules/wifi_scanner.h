#pragma once
#include "module_interface.h"
#include "../icons.h"
#include "module_registry.h"
#include "../config.h"

// Scan record
struct ScanRecord {
    char     ssid[33];
    uint8_t  bssid[6];
    uint8_t  channel;
    int8_t   rssi;
    uint8_t  encType;    // 0=Open, 1=WEP, 2=WPA, 3=WPA2, 4=WPA3/WPA2-Enterprise
};

class WifiScanner : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "WiFi Scanner"; }
    const char* getTitle() const override    { return "WiFi Scanner"; }
    const unsigned char* getIcon() const override { return icon_wifi_scanner; }
    uint8_t     getId() const override       { return 0; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

    // Other modules can use this to get scan results
    static const ScanRecord* getResults(uint8_t& count);

private:
    enum State { IDLE, SCANNING, RESULTS };
    State state;

    static ScanRecord results[WIFI_SCAN_MAX_AP];
    static uint8_t resultCount;

    uint8_t cursor;
    uint8_t scrollOffset;
    uint32_t scanStart;

    void startScan();
    void sortByRssi();
    const char* encTypeStr(uint8_t enc);
};

// Factory function
ModuleInterface* createWifiScanner();
