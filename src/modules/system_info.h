#pragma once
#include "module_interface.h"

class SystemInfo : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 7; }
    const char* getName() const override     { return "系统信息"; }
    const char* getTitle() const override    { return "系统信息"; }
    uint8_t     getId() const override       { return 80; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint32_t freeHeap;
    uint32_t totalHeap;
    uint32_t flashSize;
    uint32_t cpuFreq;
    uint32_t uptimeSec;
    char     macStr[20];
    char     ipStr[20];
    char     sdkVer[16];
    uint32_t lastRefresh;

    void refreshData();
};

ModuleInterface* createSystemInfo();
