#include "../chinese_glyphs.h"
#include "auto_config.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createAutoConfig() { return new AutoConfig(); }

void AutoConfig::init() {
    autoStartModule = 0;
    screenBrightness = 255;
    wifiOnBoot = true;
    debugMode = false;
    selectedSetting = 0;

    strcpy(settingNames[0], "自启动");
    strcpy(settingNames[1], "亮度");
    strcpy(settingNames[2], "开机连WiFi");
    strcpy(settingNames[3], "调试模式");
}

void AutoConfig::enter() {
    selectedSetting = 0;
    loadConfig();
    displayMgr.setDirty();
}

void AutoConfig::exit() {
    saveConfig();
}

void AutoConfig::loadConfig() {
    if (!lfsExists(AC_CONFIG_PATH)) return;

    File f = LittleFS.open(AC_CONFIG_PATH, "r");
    if (!f) return;

    if (f.available() >= 4) {
        autoStartModule  = f.read();
        screenBrightness = f.read();
        wifiOnBoot       = f.read();
        debugMode        = f.read();
    }
    f.close();
}

void AutoConfig::saveConfig() {
    File f = LittleFS.open(AC_CONFIG_PATH, "w");
    if (!f) return;

    f.write(autoStartModule);
    f.write(screenBrightness);
    f.write(wifiOnBoot ? 1 : 0);
    f.write(debugMode ? 1 : 0);
    f.close();
}

const char* AutoConfig::getSettingValue(uint8_t idx, char* buf, size_t sz) {
    switch (idx) {
        case 0:
            if (autoStartModule == 0) return "无";
            snprintf(buf, sz, "ID:%u", autoStartModule);
            return buf;
        case 1:
            snprintf(buf, sz, "%u", screenBrightness);
            return buf;
        case 2:
            return wifiOnBoot ? "开" : "关";
        case 3:
            return debugMode ? "开" : "关";
        default: return "?";
    }
}

void AutoConfig::toggleSetting(uint8_t idx) {
    switch (idx) {
        case 0:
            autoStartModule = (autoStartModule == 0) ? 1 : 0;
            break;
        case 1:
            if (screenBrightness >= 200) screenBrightness = 100;
            else if (screenBrightness >= 100) screenBrightness = 50;
            else screenBrightness = 255;
            break;
        case 2:
            wifiOnBoot = !wifiOnBoot;
            break;
        case 3:
            debugMode = !debugMode;
            break;
    }
    saveConfig();
}

void AutoConfig::update() {}

void AutoConfig::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_UP_SHORT:
            if (selectedSetting > 0) selectedSetting--;
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (selectedSetting < AC_MAX_SETTINGS - 1) selectedSetting++;
            displayMgr.setDirty();
            break;
        case BTN_OK_SHORT:
            toggleSetting(selectedSetting);
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void AutoConfig::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    uint8_t y = 24;
    char buf[32];

    for (uint8_t i = 0; i < AC_MAX_SETTINGS; i++) {
        u8g2.setFont(FONT_BODY);
        const char* val = getSettingValue(i, buf, sizeof(buf));

        snprintf(buf, sizeof(buf), "%c %s: %s",
                 (i == selectedSetting) ? '>' : ' ',
                 settingNames[i], val);
        drawCN(u8g2, 2, y, buf);
        y += 10;
    }

    u8g2.setFont(FONT_DATA);
}
