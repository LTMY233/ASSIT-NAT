#include "hex_convert.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createHexConvert() { return new HexConvert(); }

char HexConvert::cycleChar(char c, bool up) {
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

static uint8_t hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

void HexConvert::init() {
    hexToAscii = true;
    strCopySafe(input, "48656C6C6F", sizeof(input));
    inputLen = strlen(input);
    editPos = 0;
    output[0] = '\0';
    outputLen = 0;
    computed = false;
}

void HexConvert::enter() {
    editPos = 0;
    computed = false;
    output[0] = '\0';
    outputLen = 0;
    displayMgr.setDirty();
}

void HexConvert::exit() {}

void HexConvert::compute() {
    if (hexToAscii) {
        // Hex string to ASCII
        outputLen = 0;
        for (uint8_t i = 0; i + 1 < inputLen && outputLen < sizeof(output) - 1; i += 2) {
            char hi = input[i];
            char lo = input[i + 1];
            if (isxdigit(hi) && isxdigit(lo)) {
                output[outputLen++] = (hexVal(hi) << 4) | hexVal(lo);
            }
        }
        output[outputLen] = '\0';
    } else {
        // ASCII to Hex string
        outputLen = 0;
        for (uint8_t i = 0; i < inputLen && outputLen < sizeof(output) - 2; i++) {
            output[outputLen++] = "0123456789ABCDEF"[(input[i] >> 4) & 0x0F];
            output[outputLen++] = "0123456789ABCDEF"[input[i] & 0x0F];
        }
        output[outputLen] = '\0';
    }
    computed = true;
}

void HexConvert::update() {}

void HexConvert::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            compute();
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            input[editPos] = cycleChar(input[editPos], true);
            computed = false;
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            input[editPos] = cycleChar(input[editPos], false);
            computed = false;
            displayMgr.setDirty();
            break;
        case BTN_OK_LONG:
            editPos = (editPos + 1) % inputLen;
            displayMgr.setDirty();
            break;
        case BTN_UP_DOUBLE:
            if (inputLen < 46) {
                input[inputLen] = ' ';
                input[inputLen + 1] = '\0';
                editPos = inputLen;
                inputLen++;
            }
            computed = false;
            displayMgr.setDirty();
            break;
        case BTN_DOWN_DOUBLE:
            hexToAscii = !hexToAscii;
            computed = false;
            output[0] = '\0';
            outputLen = 0;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void HexConvert::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Hex Convert");

    // Mode
    char modeBuf[20];
    snprintf(modeBuf, sizeof(modeBuf), "%s", hexToAscii ? "Hex -> ASCII" : "ASCII -> Hex");
    u8g2.drawStr(0, 22, modeBuf);

    u8g2.drawHLine(0, 26, OLED_WIDTH);

    // Input
    u8g2.setFont(FONT_BODY);
    char inDisp[22];
    if (inputLen > 20) {
        strncpy(inDisp, input, 19); inDisp[19] = '~'; inDisp[20] = '\0';
    } else {
        strCopySafe(inDisp, input, sizeof(inDisp));
    }
    u8g2.drawStr(0, 34, inDisp);

    // Output
    if (computed) {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(0, 46, "Out:");
        char outDisp[22];
        if (outputLen > 18) {
            strncpy(outDisp, output, 17); outDisp[17] = '~'; outDisp[18] = '\0';
        } else {
            strCopySafe(outDisp, output, sizeof(outDisp));
        }
        u8g2.drawStr(20, 46, outDisp);
    }

    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 63, "OK=Conv  DblUD=Mode");
}
