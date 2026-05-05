#pragma once
#include "module_interface.h"
#include "../icons.h"

#define RF433_RAW_MAX_BITS 64

class Rf433RawTx : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName() const override     { return "433MHz Raw TX"; }
    const char* getTitle() const override    { return "433MHz Raw TX"; }
    uint8_t     getId() const override       { return 73; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    unsigned long code;
    uint8_t        bitLength;
    uint8_t        protocol;
    uint8_t        pulseLengthUs;
    uint8_t        editPos;
    uint8_t        editMode;  // 0=bit, 1=proto, 2=pulse
    uint32_t       txCount;
    bool           sent;

    void bitsToString(char* buf, size_t bufsize);
};

ModuleInterface* createRf433RawTx();
