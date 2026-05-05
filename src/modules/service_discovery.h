#pragma once
#include "module_interface.h"
#include "../icons.h"
#include <ESP8266WiFi.h>

#define SD_MAX_RESULTS 20

struct ServiceInfo {
    IPAddress ip;
    uint16_t port;
    const char* name;
};

class ServiceDiscovery : public ModuleInterface {
public:
    void init() override;
    void enter() override;
    void exit() override;
    void update() override;
    void draw(U8G2& u8g2) override;
    void handleButton(ButtonEvent ev) override;

    uint8_t     getCategory() const override { return 5; }
    const char* getName() const override     { return "Service Discovery"; }
    const char* getTitle() const override    { return "Service Discovery"; }
    const unsigned char* getIcon() const override { return icon_service; }
    uint8_t     getId() const override       { return 65; }
    RefreshMode getRefreshMode() const override { return REFRESH_ON_DEMAND; }

private:
    ServiceInfo results[SD_MAX_RESULTS];
    uint8_t  resultCount;
    bool     scanning;
    uint8_t  scanMode;   // 0=Telnet/SSH, 1=Printer, 2=Media, 3=Modbus
    uint8_t  cursor;
    uint8_t  scanIdx;
    uint32_t lastScan;
    IPAddress subnetBase;

    static const uint16_t telnetSshPorts[];
    static const uint16_t printerPorts[];
    static const uint16_t mediaServerPorts[];
    static const uint16_t modbusPorts[];

    void startScan(uint8_t mode);
    const char* getServiceName(uint16_t port, uint8_t mode);
};

ModuleInterface* createServiceDiscovery();
