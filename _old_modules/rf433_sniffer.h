#pragma once
#include "module_interface.h"
#include "../icons.h"

#define RF433_SNIFF_MAX_CODES 20

struct Rf433Code {
    unsigned long value;
    uint8_t        bitLength;
    uint8_t        protocol;
    uint32_t       timestamp;
};

class Rf433Sniffer : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName() const override     { return "433MHz Sniffer"; }
    const char* getTitle() const override    { return "433MHz Sniffer"; }
    uint8_t     getId() const override       { return 70; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    Rf433Code codes[RF433_SNIFF_MAX_CODES];
    uint8_t   codeCount;
    uint8_t   cursor;
    uint32_t  totalCaptured;
    bool      running;

    void addCode(unsigned long value, uint8_t bitLen, uint8_t proto);
    void codeToBinary(unsigned long value, uint8_t bits, char* buf, size_t bufsize);
};

ModuleInterface* createRf433Sniffer();
