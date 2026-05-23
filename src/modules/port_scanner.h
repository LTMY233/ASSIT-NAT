#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <ESP8266WiFi.h>

#define PORT_MAX_RESULTS 30
#define PORT_LIST_SIZE 22

struct PortResult {
    uint16_t port;
    bool     open;
    const char* service;
};

class PortScanner : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 0; }
    const char* getName() const override     { return "端口扫描器"; }
    const char* getTitle() const override    { return "端口扫描器"; }
    const unsigned char* getIcon() const override { return icon_port; }
    uint8_t     getId() const override       { return 5; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    PortResult results[PORT_MAX_RESULTS];
    uint8_t  resultCount;
    uint8_t  scanIdx;
    uint8_t  cursor;
    bool     scanning;
    IPAddress targetIP;
    uint32_t lastScan;

    static const uint16_t portList[PORT_LIST_SIZE];
    static const char* getServiceName(uint16_t port);

    void startScan(IPAddress ip);
};

ModuleInterface* createPortScanner();
