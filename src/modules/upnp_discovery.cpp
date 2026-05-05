#include "upnp_discovery.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createUpnpDiscovery() { return new UpnpDiscovery(); }

void UpnpDiscovery::init() {
    memset(devices, 0, sizeof(devices));
    deviceCount = 0; scanning = false; scanStart = 0; cursor = 0;
}

void UpnpDiscovery::enter() {
    deviceCount = 0; cursor = 0; scanning = false;
    sendMsearch();
    displayMgr.setDirty();
}

void UpnpDiscovery::exit() {
    udp.stop();
}

void UpnpDiscovery::sendMsearch() {
    const char* request =
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "MAN: \"ssdp:discover\"\r\n"
        "MX: 3\r\n"
        "ST: ssdp:all\r\n"
        "\r\n";

    udp.begin(1900);
    udp.beginPacket(IPAddress(239, 255, 255, 250), 1900);
    udp.write((const uint8_t*)request, strlen(request));
    udp.endPacket();

    scanStart = millis();
    scanning = true;
}

void UpnpDiscovery::parseResponse(uint8_t* buf, uint16_t len) {
    if (deviceCount >= UPNP_MAX_DEVICES) return;

    // Parse HTTP response header
    char* data = (char*)buf;
    data[len] = '\0';

    UpnpDevice& dev = devices[deviceCount];
    bool hasData = false;

    char* line = strtok(data, "\r\n");
    while (line) {
        if (strncasecmp(line, "LOCATION:", 9) == 0) {
            const char* val = line + 9;
            while (*val == ' ') val++;
            strCopySafe(dev.location, val, sizeof(dev.location));
            hasData = true;
        } else if (strncasecmp(line, "SERVER:", 7) == 0) {
            const char* val = line + 7;
            while (*val == ' ') val++;
            strCopySafe(dev.server, val, sizeof(dev.server));
            hasData = true;
        } else if (strncasecmp(line, "USN:", 4) == 0) {
            const char* val = line + 4;
            while (*val == ' ') val++;
            strCopySafe(dev.usn, val, sizeof(dev.usn));
            hasData = true;
        } else if (strncasecmp(line, "ST:", 3) == 0) {
            const char* val = line + 3;
            while (*val == ' ') val++;
            strCopySafe(dev.st, val, sizeof(dev.st));
            hasData = true;
        }
        line = strtok(nullptr, "\r\n");
    }

    if (hasData) deviceCount++;
}

void UpnpDiscovery::update() {
    if (!scanning) return;

    if (udp.parsePacket()) {
        uint8_t buf[512];
        int len = udp.read(buf, 512);
        if (len > 0) parseResponse(buf, len);
        displayMgr.setDirty();
    }

    if (millis() - scanStart > 6000) {
        scanning = false;
        udp.stop();
        displayMgr.setDirty();
    }
}

void UpnpDiscovery::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT && cursor > 0) { cursor--; displayMgr.setDirty(); }
    if (ev == BTN_DOWN_SHORT && cursor < deviceCount - 1) { cursor++; displayMgr.setDirty(); }
    if (ev == BTN_OK_SHORT && !scanning) { deviceCount = 0; sendMsearch(); displayMgr.setDirty(); }
}

void UpnpDiscovery::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "UPnP/SSDP Discovery");

    if (scanning) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(2, 30, "Send M-SEARCH...");
        return;
    }

    if (deviceCount == 0) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(10, 35, "No UPnP devices");
    } else {
        u8g2.setFont(FONT_DATA);
        for (uint8_t i = 0; i < deviceCount && i < 2; i++) {
            uint8_t y = 24 + i * 15;
            if (i == cursor) {
                u8g2.drawBox(0, y - 12, OLED_WIDTH, 15);
                u8g2.setDrawColor(0);
            }
            u8g2.drawStr(2, y, devices[i].server[0] ? devices[i].server : devices[i].usn);
            if (i == cursor) u8g2.setDrawColor(1);
        }
    }

    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 63, "OK to rescan");
}
