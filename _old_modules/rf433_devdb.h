#pragma once
#include "module_interface.h"
#include "../icons.h"

#define RF433_DB_MAX_DEVICES 24

struct Rf433Device {
    char          name[20];
    unsigned long code;
    uint8_t       bitLength;
    uint8_t       protocol;
};

class Rf433DevDb : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 6; }
    const char* getName() const override     { return "433MHz Dev DB"; }
    const char* getTitle() const override    { return "433MHz Dev DB"; }
    uint8_t     getId() const override       { return 74; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    Rf433Device devices[RF433_DB_MAX_DEVICES];
    uint8_t  deviceCount;
    uint8_t  cursor;
    bool     detailView;
    uint32_t txCount;

    void loadDeviceDB();
    void codeToBinary(unsigned long value, uint8_t bits, char* buf, size_t bufsize);
};

ModuleInterface* createRf433DevDb();
