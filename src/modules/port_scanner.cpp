#include "../chinese_glyphs.h"
#include "port_scanner.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>

const uint16_t PortScanner::portList[PORT_LIST_SIZE] = {
    21, 22, 23, 25, 53, 80, 110, 123, 137, 139,
    143, 443, 445, 502, 631, 993, 1433, 1883, 3306, 3389,
    8080, 8443
};

ModuleInterface* createPortScanner() { return new PortScanner(); }

const char* PortScanner::getServiceName(uint16_t port) {
    switch (port) {
        case 21: return "FTP"; case 22: return "SSH";
        case 23: return "Telnet"; case 25: return "SMTP";
        case 53: return "DNS"; case 80: return "HTTP";
        case 110: return "POP3"; case 123: return "NTP";
        case 137: return "NetBIOS"; case 139: return "SMB";
        case 143: return "IMAP"; case 443: return "HTTPS";
        case 445: return "SMB"; case 502: return "Modbus";
        case 631: return "IPP"; case 993: return "IMAPS";
        case 1433: return "MSSQL"; case 1883: return "MQTT";
        case 3306: return "MySQL"; case 3389: return "RDP";
        case 8080: return "HTTP-Alt"; case 8443: return "HTTPS-Alt";
        default: return "?";
    }
}

void PortScanner::init() {
    memset(results, 0, sizeof(results));
    resultCount = 0; scanIdx = 0; cursor = 0;
    scanning = false; lastScan = 0;
}

void PortScanner::enter() {
    resultCount = 0; cursor = 0;
    targetIP = WiFi.gatewayIP();  // default scan gateway
    lastScan = 0;
    startScan(targetIP);
    displayMgr.setDirty();
}

void PortScanner::exit() {}

void PortScanner::startScan(IPAddress ip) {
    targetIP = ip;
    scanning = true;
    scanIdx = 0;
    resultCount = 0;
    lastScan = millis();
}

void PortScanner::update() {
    if (!scanning) return;

    // Scan one port at a time (non-blocking)
    if (scanIdx < PORT_LIST_SIZE && millis() - lastScan > 50) {
        uint16_t port = portList[scanIdx];
        WiFiClient client;
        client.setTimeout(1000);
        bool open = client.connect(targetIP, port);
        if (client.connected()) {
            results[resultCount].port = port;
            results[resultCount].open = true;
            results[resultCount].service = getServiceName(port);
            resultCount++;
            client.stop();
        } else if (open) {
            // Connected then closed? count as open
            results[resultCount].port = port;
            results[resultCount].open = true;
            results[resultCount].service = getServiceName(port);
            resultCount++;
        }
        client.stop();
        scanIdx++;
        lastScan = millis();
        displayMgr.setDirty();
    }

    if (scanIdx >= PORT_LIST_SIZE) {
        scanning = false;
        displayMgr.setDirty();
    }
}

void PortScanner::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT && cursor > 0) { cursor--; displayMgr.setDirty(); }
    if (ev == BTN_DOWN_SHORT && cursor < resultCount - 1) { cursor++; displayMgr.setDirty(); }
    if (ev == BTN_OK_SHORT && !scanning) { startScan(targetIP); displayMgr.setDirty(); }
}

void PortScanner::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    if (scanning) {
        u8g2.setFont(FONT_BODY);
        char buf[32];
        snprintf(buf, sizeof(buf), "Scan %d/%d : %d", scanIdx, PORT_LIST_SIZE, portList[scanIdx]);
        u8g2.drawStr(2, 30, buf);
        uint8_t prog = scanIdx * OLED_WIDTH / PORT_LIST_SIZE;
        u8g2.drawFrame(0, 38, OLED_WIDTH, 5);
        u8g2.drawBox(0, 38, prog, 5);
        return;
    }

    if (resultCount == 0) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 10, 35, "无开放端口");
    } else {
        u8g2.setFont(FONT_DATA);
        for (uint8_t i = 0; i < resultCount && i < 2; i++) {
            uint8_t y = 39 + i * 15;
            if (i == cursor) {
                u8g2.drawBox(0, y - 12, OLED_WIDTH, 15);
                u8g2.setDrawColor(0);
            }
            char buf[30];
            snprintf(buf, sizeof(buf), "%-5d %s", results[i].port, results[i].service);
            u8g2.drawStr(2, y, buf);
            if (i == cursor) u8g2.setDrawColor(1);
        }
    }

    u8g2.setFont(FONT_DATA);
    char buf[30];
    snprintf(buf, sizeof(buf), "Tgt:%d.%d.%d.%d  %d open",
             targetIP[0], targetIP[1], targetIP[2], targetIP[3], resultCount);
    u8g2.drawStr(0, 63, buf);
}
