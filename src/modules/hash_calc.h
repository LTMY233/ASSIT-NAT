#pragma once
#include "module_interface.h"

class HashCalc : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 3; }
    const char* getName() const override     { return "Hash Calc"; }
    const char* getTitle() const override    { return "Hash Calc"; }
    uint8_t     getId() const override       { return 35; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }
    bool        canRunOffline() const override { return true; }

private:
    char     input[48];
    uint8_t  inputLen;
    uint8_t  editPos;
    uint8_t  hashMode;        // 0=XOR, 1=FNV32
    char     hashStr[16];
    bool     computed;

    static char cycleChar(char c, bool up);
    void computeHash();
    uint32_t xorHash(const char* str, uint8_t len);
    uint32_t fnv32Hash(const char* str, uint8_t len);
};

ModuleInterface* createHashCalc();
