#include "mdns_enum.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createMdnsEnum() { return new MdnsEnum(); }

void MdnsEnum::init() {
    memset(services, 0, sizeof(services));
    serviceCount = 0; scanning = false; scanStart = 0; cursor = 0;
}

void MdnsEnum::enter() {
    serviceCount = 0; cursor = 0; scanning = false;
    sendQuery();
    displayMgr.setDirty();
}

void MdnsEnum::exit() {
    udp.stop();
}

void MdnsEnum::sendQuery() {
    // Build mDNS query: _services._dns-sd._udp.local PTR
    // Simplified DNS query packet
    uint8_t query[] = {
        0x00, 0x00,  // Transaction ID (mDNS must be 0)
        0x00, 0x00,  // Flags: Standard query
        0x00, 0x01,  // Questions: 1
        0x00, 0x00,  // Answer RRs
        0x00, 0x00,  // Authority RRs
        0x00, 0x00,  // Additional RRs
        // Query: _services._dns-sd._udp.local
        0x09, '_', 's', 'e', 'r', 'v', 'i', 'c', 'e', 's',
        0x07, '_', 'd', 'n', 's', '-', 's', 'd',
        0x04, '_', 'u', 'd', 'p',
        0x05, 'l', 'o', 'c', 'a', 'l',
        0x00,
        0x00, 0x0C,  // Type: PTR
        0x00, 0x01,  // Class: IN
    };

    udp.begin(5353);
    udp.beginPacket(IPAddress(224, 0, 0, 251), 5353);
    udp.write(query, sizeof(query));
    udp.endPacket();

    scanStart = millis();
    scanning = true;
}

void MdnsEnum::parseResponse(uint8_t* buf, uint16_t len) {
    if (len < 12) return;
    uint16_t qdCount = ((uint16_t)buf[4] << 8) | buf[5];
    uint16_t anCount = ((uint16_t)buf[6] << 8) | buf[7];

    if (anCount == 0) return;

    // Skip Questions
    uint16_t pos = 12;
    for (uint16_t q = 0; q < qdCount && pos < len; q++) {
        while (pos < len && buf[pos] != 0) pos++;
        pos += 5;  // null + type(2) + class(2)
    }

    // Parse Answers
    for (uint16_t a = 0; a < anCount && serviceCount < MDNS_MAX_SERVICES && pos < len; a++) {
        if (buf[pos] == 0xC0) {
            pos += 2;  // Compressed pointer
        } else {
            while (pos < len && buf[pos] != 0) pos++;
            pos += 1;
        }

        if (pos + 10 > len) break;
        uint16_t type = ((uint16_t)buf[pos] << 8) | buf[pos + 1];
        uint16_t rdLen = ((uint16_t)buf[pos + 8] << 8) | buf[pos + 9];
        pos += 10;

        if (type == 12 && rdLen > 0 && pos + rdLen <= len) {  // PTR
            // Extract service type name
            uint16_t rdp = pos;
            char tmpName[32] = {0};
            uint8_t ni = 0;
            while (rdp < pos + rdLen && rdp < len && ni < 31) {
                uint8_t segLen = buf[rdp];
                if (segLen == 0xC0) break;
                if (segLen == 0) break;
                rdp++;
                if (ni > 0) tmpName[ni++] = '.';
                for (uint8_t s = 0; s < segLen && ni < 31 && rdp < pos + rdLen; s++) {
                    tmpName[ni++] = (char)buf[rdp++];
                }
            }
            if (ni > 0) {
                strCopySafe(services[serviceCount].type, tmpName, 32);
                services[serviceCount].port = 0;
                strCopySafe(services[serviceCount].name, "", 32);
                serviceCount++;
            }
        }
        pos += rdLen;
    }
}

void MdnsEnum::update() {
    if (!scanning) return;

    if (udp.parsePacket()) {
        uint8_t buf[512];
        int len = udp.read(buf, 512);
        if (len > 0) parseResponse(buf, len);
        displayMgr.setDirty();
    }

    if (millis() - scanStart > 5000) {
        scanning = false;
        udp.stop();
        displayMgr.setDirty();
    }
}

void MdnsEnum::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT && cursor > 0) { cursor--; displayMgr.setDirty(); }
    if (ev == BTN_DOWN_SHORT && cursor < serviceCount - 1) { cursor++; displayMgr.setDirty(); }
    if (ev == BTN_OK_SHORT && !scanning) { serviceCount = 0; sendQuery(); displayMgr.setDirty(); }
}

void MdnsEnum::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "mDNS Enum");

    if (scanning) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(2, 30, "Searching...");
        return;
    }

    if (serviceCount == 0) {
        u8g2.setFont(FONT_BODY);
        u8g2.drawStr(10, 35, "None found");
    } else {
        u8g2.setFont(FONT_DATA);
        for (uint8_t i = 0; i < serviceCount && i < 2; i++) {
            uint8_t y = 24 + i * 15;
            if (i == cursor) {
                u8g2.drawBox(0, y - 12, OLED_WIDTH, 15);
                u8g2.setDrawColor(0);
            }
            u8g2.drawStr(2, y, services[i].type);
            if (i == cursor) u8g2.setDrawColor(1);
        }
    }

    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 63, "Press OK to rescan");
}
