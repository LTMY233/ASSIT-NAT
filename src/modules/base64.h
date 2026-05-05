#pragma once
#include "module_interface.h"

class Base64Tool : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 3; }
    const char* getName() const override     { return "Base64 Codec"; }
    const char* getTitle() const override    { return "Base64"; }
    uint8_t     getId() const override       { return 33; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }
    bool        canRunOffline() const override { return true; }

private:
    bool     encodeMode;       // true=encode, false=decode
    char     input[48];
    uint8_t  inputLen;
    uint8_t  editPos;
    char     output[64];
    uint8_t  outputLen;
    bool     computed;

    static char cycleChar(char c, bool up);
    void compute();
};

ModuleInterface* createBase64Tool();
