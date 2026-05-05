#pragma once
#include "module_interface.h"
#include <ESP8266WiFi.h>

class HttpClient : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "HTTP Client"; }
    const char* getTitle() const override    { return "HTTP Client"; }
    uint8_t     getId() const override       { return 12; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    char     url[64];
    uint8_t  urlIdx;
    uint8_t  editPos;
    uint8_t  urlLen;
    int      statusCode;
    int      responseSize;
    bool     loaded;
    char     status[20];
    bool     fetching;

    static const char* presetUrls[4];
    static char cycleChar(char c, bool up);

    void doFetch();
    void selectPreset(uint8_t idx);
};

ModuleInterface* createHttpClient();
