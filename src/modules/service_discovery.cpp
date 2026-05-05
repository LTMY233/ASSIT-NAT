#include "service_discovery.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

const uint16_t ServiceDiscovery::telnetSshPorts[] = {22, 23};
const uint16_t ServiceDiscovery::printerPorts[] = {631, 515, 9100};
const uint16_t ServiceDiscovery::mediaServerPorts[] = {32400, 8200, 8096, 9090};
const uint16_t ServiceDiscovery::modbusPorts[] = {502};

ModuleInterface* createServiceDiscovery() { return new ServiceDiscovery(); }

void ServiceDiscovery::init() {
    memset(results, 0, sizeof(results));
    resultCount = 0; scanning = false;
    scanMode = 0; cursor = 0; scanIdx = 0; lastScan = 0;
}

void ServiceDiscovery::enter() {
    resultCount = 0; cursor = 0; scanMode = 0;
    subnetBase = WiFi.localIP();
    startScan(scanMode);
    displayMgr.setDirty();
}

void ServiceDiscovery::exit() {}

const char* ServiceDiscovery::getServiceName(uint16_t port, uint8_t mode) {
    if (port == 22) return "SSH";
    if (port == 23) return "Telnet";
    if (port == 631) return "IPP";
    if (port == 515) return "LPD";
    if (port == 9100) return "JetDirect";
    if (port == 32400) return "Plex";
    if (port == 8200) return "DLNA";
    if (port == 8096) return "Jellyfin";
    if (port == 9090) return "Emby";
    if (port == 502) return "Modbus";
    return "?";
}

void ServiceDiscovery::startScan(uint8_t mode) {
    scanMode = mode;
    scanning = true;
    scanIdx = 0;
    resultCount = 0;
    lastScan = millis();
}

void ServiceDiscovery::update() {
    if (!scanning) return;

    const uint16_t* ports = nullptr;
    uint8_t portCount = 0;
    switch (scanMode) {
        case 0: ports = telnetSshPorts; portCount = 2; break;
        case 1: ports = printerPorts; portCount = 3; break;
        case 2: ports = mediaServerPorts; portCount = 4; break;
        case 3: ports = modbusPorts; portCount = 1; break;
    }

    // Scan LAN IPs (simplified: near gateway)
    if (scanIdx < 10 && millis() - lastScan > 100) {
        IPAddress ip = subnetBase;
        ip[3] = scanIdx + 1;

        for (uint8_t p = 0; p < portCount; p++) {
            WiFiClient client;
            client.setTimeout(500);
            if (client.connect(ip, ports[p])) {
                if (resultCount < SD_MAX_RESULTS) {
                    results[resultCount].ip = ip;
                    results[resultCount].port = ports[p];
                    results[resultCount].name = getServiceName(ports[p], scanMode);
                    resultCount++;
                }
                client.stop();
            }
        }
        scanIdx++;
        lastScan = millis();
        displayMgr.setDirty();
    }

    if (scanIdx >= 10) {
        scanning = false;
        displayMgr.setDirty();
    }
}

void ServiceDiscovery::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_UP_SHORT:
            if (scanMode > 0) scanMode--;
            if (!scanning) startScan(scanMode);
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (scanMode < 3) scanMode++;
            if (!scanning) startScan(scanMode);
            displayMgr.setDirty();
            break;
        case BTN_OK_SHORT:
            if (!scanning) startScan(scanMode);
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void ServiceDiscovery::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Service Discovery Scan");

    u8g2.setFont(FONT_DATA);
    const char* modeNames[] = {"Telnet/SSH", "Printer", "Media Server", "Modbus"};
    char buf[30];
    snprintf(buf, sizeof(buf), "Mode:%s", modeNames[scanMode]);
    u8g2.drawStr(2, 24, buf);

    if (scanning) {
        u8g2.drawStr(2, 30, "Scanning...");
    } else if (resultCount == 0) {
        u8g2.drawStr(10, 35, "No services found");
    } else {
        for (uint8_t i = 0; i < resultCount && i < 2; i++) {
            uint8_t y = 39 + i * 15;
            snprintf(buf, sizeof(buf), "%d.%d.%d.%d:%d %s",
                     results[i].ip[0], results[i].ip[1],
                     results[i].ip[2], results[i].ip[3],
                     results[i].port, results[i].name);
            u8g2.drawStr(0, y, buf);
        }
    }

    u8g2.drawStr(0, 63, "Up/Dn mode  OK rescan");
}
