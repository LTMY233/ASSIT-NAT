#pragma once
#include "module_interface.h"

#define AC_MAX_SETTINGS 4
#define AC_CONFIG_PATH  "/autocfg.dat"

class AutoConfig : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 7; }
    const char* getName() const override     { return "Auto Config"; }
    const char* getTitle() const override    { return "Boot Config"; }
    uint8_t     getId() const override       { return 86; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    uint8_t autoStartModule;  // 0 = none
    uint8_t screenBrightness; // 0-255
    bool    wifiOnBoot;
    bool    debugMode;

    char    settingNames[AC_MAX_SETTINGS][20];
    uint8_t selectedSetting;

    void loadConfig();
    void saveConfig();
    void toggleSetting(uint8_t idx);
    const char* getSettingValue(uint8_t idx, char* buf, size_t sz);
};

ModuleInterface* createAutoConfig();
