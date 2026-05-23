#include "../chinese_glyphs.h"
#include "dns_lookup.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createDnsLookup() { return new DnsLookup(); }

char DnsLookup::cycleChar(char c, bool up) {
    if (up) {
        if (c < 32 || c > 126) return ' ';
        if (c >= 126) return ' ';
        return c + 1;
    } else {
        if (c < 32 || c > 126) return 'a';
        if (c <= 32) return 126;
        return c - 1;
    }
}

void DnsLookup::init() {
    strCopySafe(hostname, "google.com", sizeof(hostname));
    hostLen = strlen(hostname);
    editPos = 0;
    resolved = false;
    resolving = false;
    resultStr[0] = '\0';
}

void DnsLookup::enter() {
    editPos = 0;
    resolved = false;
    resolving = false;
    resultStr[0] = '\0';
    displayMgr.setDirty();
}

void DnsLookup::exit() {}

void DnsLookup::doLookup() {
    resolved = false;
    resolving = true;
    resolvedIP = WiFi.hostByName(hostname, resolvedIP);
    if (resolvedIP[0] != 0) {
        resolved = true;
        ipToStr(resolvedIP, resultStr, sizeof(resultStr));
    } else {
        // Try alternate API
        IPAddress ip;
        if (WiFi.hostByName(hostname, ip) && ip[0] != 0) {
            resolved = true;
            resolvedIP = ip;
            ipToStr(resolvedIP, resultStr, sizeof(resultStr));
        } else {
            strCopySafe(resultStr, "Failed", sizeof(resultStr));
        }
    }
    resolving = false;
}

void DnsLookup::update() {}

void DnsLookup::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            doLookup();
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            hostname[editPos] = cycleChar(hostname[editPos], true);
            resolved = false;
            resultStr[0] = '\0';
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            hostname[editPos] = cycleChar(hostname[editPos], false);
            resolved = false;
            resultStr[0] = '\0';
            displayMgr.setDirty();
            break;
        case BTN_OK_LONG:
            editPos = (editPos + 1) % hostLen;
            displayMgr.setDirty();
            break;
        case BTN_UP_DOUBLE:
            // Add a character at end
            if (hostLen < 46) {
                hostname[hostLen] = ' ';
                hostname[hostLen + 1] = '\0';
                editPos = hostLen;
                hostLen++;
            }
            displayMgr.setDirty();
            break;
        case BTN_DOWN_DOUBLE:
            // Remove character at end
            if (hostLen > 1) {
                hostLen--;
                hostname[hostLen] = '\0';
                if (editPos >= hostLen) editPos = hostLen - 1;
            }
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void DnsLookup::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // Hostname input field
    char lineBuf[50];
    snprintf(lineBuf, sizeof(lineBuf), "Host: %s", hostname);
    u8g2.drawStr(0, 24, lineBuf);

    // Cursor indicator
    uint8_t cursorX = 0 + u8g2.getStrWidth("Host: ");
    for (uint8_t i = 0; i < editPos && i < hostLen; i++) {
        char tmp[2] = {hostname[i], '\0'};
        cursorX += u8g2.getStrWidth(tmp);
    }
    u8g2.drawStr(cursorX, 24, "_");

    // Divider line
    u8g2.drawHLine(0, 30, OLED_WIDTH);

    // Result
    u8g2.setFont(FONT_DATA);
    if (resolving) {
        drawCN(u8g2, 0, 44, "解析中...");
    } else if (resolved) {
        u8g2.setFont(FONT_BIG);
        uint8_t tw = u8g2.getStrWidth(resultStr);
        u8g2.drawStr((OLED_WIDTH - tw) / 2, 48, resultStr);
    } else {
        drawCN(u8g2, 0, 44, "OK=解析 上下=编辑");
        u8g2.drawStr(2, 55, "HoldOK=next Dbl=+/-");
    }

    // Footer
    u8g2.setFont(FONT_DATA);
}
