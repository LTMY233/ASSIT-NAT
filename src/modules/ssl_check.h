#pragma once
#include "module_interface.h"
#include <ESP8266WiFi.h>

class SslCheck : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "SSL检查"; }
    const char* getTitle() const override    { return "SSL检查"; }
    uint8_t     getId() const override       { return 49; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    char     host[48];
    uint8_t  editPos;
    uint8_t  hostLen;
    bool     connecting;
    bool     connected;
    char     result[32];

    static char cycleChar(char c, bool up);
    void doCheck();
};

ModuleInterface* createSslCheck();
