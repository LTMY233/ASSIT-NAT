#pragma once
#include "module_interface.h"

class XorEncrypt : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 3; }
    const char* getName() const override     { return "XOR Encrypt"; }
    const char* getTitle() const override    { return "XOR Encrypt"; }
    uint8_t     getId() const override       { return 36; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }
    bool        canRunOffline() const override { return true; }

private:
    char     key[16];
    uint8_t  keyLen;
    char     input[48];
    uint8_t  inputLen;
    uint8_t  editField;       // 0=key, 1=input
    uint8_t  editPos;
    char     output[48];
    uint8_t  outputLen;
    bool     computed;

    static char cycleChar(char c, bool up);
    void compute();
};

ModuleInterface* createXorEncrypt();
