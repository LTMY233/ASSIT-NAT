#pragma once
#include "module_interface.h"
#include "../icons.h"

class PasswordGen : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 3; }
    const char* getName() const override     { return "Password Gen"; }
    const char* getTitle() const override    { return "Password Gen"; }
    const unsigned char* getIcon() const override { return icon_password; }
    uint8_t     getId() const override       { return 31; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    char    password[33];
    uint8_t pwdLen;
    bool    useUpper, useLower, useDigits, useSymbols;

    void generate();
};

ModuleInterface* createPasswordGen();
