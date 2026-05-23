#include "../chinese_glyphs.h"
#include "ssl_check.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <WiFiClientSecureBearSSL.h>

ModuleInterface* createSslCheck() { return new SslCheck(); }

char SslCheck::cycleChar(char c, bool up) {
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

void SslCheck::init() {
    strCopySafe(host, "google.com", sizeof(host));
    hostLen = strlen(host);
    editPos = 0;
    connecting = false;
    connected = false;
    strCopySafe(result, "OK=check", sizeof(result));
}

void SslCheck::enter() {
    editPos = 0;
    connecting = false;
    connected = false;
    strCopySafe(result, "OK=check", sizeof(result));
    displayMgr.setDirty();
}

void SslCheck::exit() {}

void SslCheck::doCheck() {
    connecting = true;
    connected = false;
    strCopySafe(result, "Connecting...", sizeof(result));
    displayMgr.setDirty();

    BearSSL::WiFiClientSecure client;
    client.setInsecure();  // Accept any certificate for testing
    client.setTimeout(8000);

    if (client.connect(host, 443)) {
        connected = true;
        snprintf(result, sizeof(result), "SSL OK: %s:443", host);
        client.stop();
    } else {
        connected = false;
        // Get more detail
        int lastErr = client.getLastSSLError();
        if (lastErr == BR_ERR_OK) {
            strCopySafe(result, "TCP OK, TLS fail", sizeof(result));
        } else {
            snprintf(result, sizeof(result), "Fail err=%d", lastErr);
        }
    }
    connecting = false;
}

void SslCheck::update() {}

void SslCheck::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            doCheck();
            break;
        case BTN_UP_SHORT:
            host[editPos] = cycleChar(host[editPos], true);
            strCopySafe(result, "OK=check", sizeof(result));
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            host[editPos] = cycleChar(host[editPos], false);
            strCopySafe(result, "OK=check", sizeof(result));
            displayMgr.setDirty();
            break;
        case BTN_OK_LONG:
            editPos = (editPos + 1) % hostLen;
            displayMgr.setDirty();
            break;
        case BTN_UP_DOUBLE:
            if (hostLen < 46) {
                host[hostLen] = ' ';
                host[hostLen + 1] = '\0';
                editPos = hostLen;
                hostLen++;
            }
            strCopySafe(result, "OK=check", sizeof(result));
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void SslCheck::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // Host input
    char lineBuf[50];
    snprintf(lineBuf, sizeof(lineBuf), "Host: %s", host);
    u8g2.drawStr(0, 24, lineBuf);

    // Cursor
    uint8_t cursorX = u8g2.getStrWidth("Host: ");
    for (uint8_t i = 0; i < editPos && i < hostLen; i++) {
        char tmp[2] = {host[i], '\0'};
        cursorX += u8g2.getStrWidth(tmp);
    }
    u8g2.drawStr(cursorX, 24, "_");

    u8g2.drawHLine(0, 30, OLED_WIDTH);

    if (connecting) {
        u8g2.setFont(FONT_DATA);
        drawCN(u8g2, 0, 44, "测试SSL/TLS...");
    } else {
        u8g2.setFont(FONT_DATA);
        // Word-wrap long result
        if (strlen(result) > 21) {
            char line1[22];
            strncpy(line1, result, 21); line1[21] = '\0';
            u8g2.drawStr(0, 44, line1);
            u8g2.drawStr(0, 55, result + 21);
        } else {
            u8g2.drawStr(0, 44, result);
        }
    }

    u8g2.setFont(FONT_DATA);
}
