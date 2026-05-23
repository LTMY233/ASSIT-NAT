#pragma once
#include "module_interface.h"
#include "../icons.h"

class TotpGenerator : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 3; }
    const char* getName() const override     { return "动态口令"; }
    const char* getTitle() const override    { return "动态口令"; }
    const unsigned char* getIcon() const override { return icon_totp; }
    uint8_t     getId() const override       { return 30; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    // Default seed (Base32), demo
    // In production, load AES-encrypted seed from LittleFS
    static const char* defaultSecret;

    uint32_t lastTotp;
    uint32_t lastCalc;
    char     code[9];
    uint8_t  codeLen;
    uint8_t  countdown;  // 30-sec countdown

    void calcTotp();
    uint32_t base32Decode(const char* base32, uint8_t* out, uint8_t maxLen);
    void hmacSha1(const uint8_t* key, uint8_t keyLen, const uint8_t* msg, uint8_t msgLen, uint8_t* digest);
};

ModuleInterface* createTotpGenerator();
