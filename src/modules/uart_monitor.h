#pragma once
#include "module_interface.h"
#include "../icons.h"

#define UART_RX_BUF_SIZE 128

class UartMonitor : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 4; }
    const char* getName() const override     { return "UART Monitor"; }
    const char* getTitle() const override    { return "UART Monitor"; }
    const unsigned char* getIcon() const override { return icon_i2c; }
    uint8_t     getId() const override       { return 55; }
    RefreshMode getRefreshMode() const override { return REFRESH_CONTINUOUS; }

private:
    uint32_t baudRate;
    uint32_t rxCount;
    uint32_t txCount;
    uint32_t lastUpdate;
    char     rxBuf[UART_RX_BUF_SIZE];
    uint8_t  rxIdx;
    bool     passthrough;
    uint8_t  baudIdx;

    static const uint32_t baudList[8];
    void applyBaud();
};

ModuleInterface* createUartMonitor();
