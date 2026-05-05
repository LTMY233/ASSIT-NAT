#pragma once
#include "module_interface.h"

class Settings : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t getId() const override { return 88; }
    uint8_t getCategory() const override { return 7; }
    const char* getName() const override { return "Settings"; }
    const char* getTitle() const override { return "Settings"; }
    const unsigned char* getIcon() const override { return nullptr; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }
    bool canRunOffline() const override { return true; }

private:
    uint8_t page;        // 0=contrast, 1=firmware, 2=author
    uint8_t contrast;
    bool    inverted;
    uint8_t cursor;

    static const uint8_t PAGE_COUNT = 3;
};

ModuleInterface* createSettings();
