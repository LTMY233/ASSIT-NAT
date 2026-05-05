#pragma once
#include "module_interface.h"
#include "../icons.h"

class NetSnapshot : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 5; }
    const char* getName() const override     { return "Net Snapshot"; }
    const char* getTitle() const override    { return "Net Snapshot"; }
    const unsigned char* getIcon() const override { return icon_snapshot; }
    uint8_t     getId() const override       { return 67; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    bool     saved;
    char     filename[24];
    uint32_t saveTime;
    uint8_t  apCount;
    int8_t   avgRssi;

    void saveSnapshot();
};

ModuleInterface* createNetSnapshot();
