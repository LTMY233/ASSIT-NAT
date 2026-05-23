#pragma once
#include "module_interface.h"

class SleepTimer : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 7; }
    const char* getName() const override     { return "休眠定时器"; }
    const char* getTitle() const override    { return "深度休眠"; }
    uint8_t     getId() const override       { return 85; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint32_t sleepSeconds;
    bool     confirmStart;
    uint32_t countdownStart;
    uint8_t  stepMode;  // 0=1s, 1=10s, 2=60s, 3=600s

    void formatDuration(char* buf, size_t sz);
};

ModuleInterface* createSleepTimer();
