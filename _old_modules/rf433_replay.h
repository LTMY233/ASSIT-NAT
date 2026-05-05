#pragma once
#include "module_interface.h"
#include "../icons.h"

#define RF433_REPLAY_MAX_CODES 16

struct Rf433SavedCode {
    unsigned long value;
    uint8_t        bitLength;
    uint8_t        protocol;
    char           name[16];
};

class Rf433Replay : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName() const override     { return "433MHz Replay"; }
    const char* getTitle() const override    { return "433MHz Replay"; }
    uint8_t     getId() const override       { return 71; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    Rf433SavedCode savedCodes[RF433_REPLAY_MAX_CODES];
    uint8_t  codeCount;
    uint8_t  cursor;
    uint32_t txCount;
};

ModuleInterface* createRf433Replay();
