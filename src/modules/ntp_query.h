#pragma once
#include "module_interface.h"
#include <WiFiUdp.h>

class NtpQuery : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "NTP Query"; }
    const char* getTitle() const override    { return "NTP Query"; }
    uint8_t     getId() const override       { return 14; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint32_t ntpTime;       // NTP epoch received
    int32_t  offset;         // offset vs local millis
    uint32_t lastSync;
    char     timeStr[12];
    bool     synced;
    bool     querying;
    char     status[24];

    void sendNtpQuery();
    bool checkNtpReply();
};

ModuleInterface* createNtpQuery();
