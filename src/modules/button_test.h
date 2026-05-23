#pragma once
#include "module_interface.h"

class ButtonTest : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 7; }
    const char* getName() const override     { return "按键测试"; }
    const char* getTitle() const override    { return "按键测试"; }
    uint8_t     getId() const override       { return 83; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    bool     upPressed;
    bool     downPressed;
    bool     okPressed;
    uint32_t upPressStart;
    uint32_t downPressStart;
    uint32_t okPressStart;
    uint32_t upDuration;
    uint32_t downDuration;
    uint32_t okDuration;
    uint32_t lastEventTime;
    char     lastEventName[16];
};

ModuleInterface* createButtonTest();
