#include "../chinese_glyphs.h"
#include "base64.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

static const char b64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

ModuleInterface* createBase64Tool() { return new Base64Tool(); }

char Base64Tool::cycleChar(char c, bool up) {
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

static int8_t b64Index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (c == '=') return -1;
    return -1;
}

void Base64Tool::init() {
    encodeMode = true;
    strCopySafe(input, "Hello", sizeof(input));
    inputLen = strlen(input);
    editPos = 0;
    output[0] = '\0';
    outputLen = 0;
    computed = false;
}

void Base64Tool::enter() {
    editPos = 0;
    computed = false;
    output[0] = '\0';
    outputLen = 0;
    displayMgr.setDirty();
}

void Base64Tool::exit() {}

void Base64Tool::compute() {
    if (encodeMode) {
        // Encode
        outputLen = 0;
        uint8_t i = 0;
        while (i < inputLen) {
            uint32_t val = 0;
            uint8_t count = 0;
            for (uint8_t j = 0; j < 3 && i < inputLen; j++, i++) {
                val = (val << 8) | (uint8_t)input[i];
                count++;
            }
            val <<= (3 - count) * 8;
            for (uint8_t j = 0; j <= count; j++) {
                if (outputLen < sizeof(output) - 1) {
                    output[outputLen++] = b64Table[(val >> (18 - j * 6)) & 0x3F];
                }
            }
            for (uint8_t j = count; j < 3 && outputLen < sizeof(output) - 1; j++) {
                output[outputLen++] = '=';
            }
        }
        output[outputLen] = '\0';
    } else {
        // Decode
        outputLen = 0;
        uint8_t i = 0;
        while (i < inputLen && input[i] != '=') {
            uint32_t val = 0;
            uint8_t count = 0;
            for (uint8_t j = 0; j < 4 && i < inputLen; j++) {
                char c = input[i];
                if (c == '=') { i++; continue; }
                int8_t idx = b64Index(c);
                if (idx >= 0) {
                    val = (val << 6) | idx;
                    count++;
                }
                i++;
            }
            if (count >= 2) {
                for (uint8_t j = 0; j < count - 1 && outputLen < sizeof(output) - 1; j++) {
                    output[outputLen++] = (val >> (16 - j * 8)) & 0xFF;
                }
            }
        }
        output[outputLen] = '\0';
    }
    computed = true;
}

void Base64Tool::update() {}

void Base64Tool::handleButton(ButtonEvent ev) {
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
            encodeMode = !encodeMode;
            computed = false;
            output[0] = '\0';
            outputLen = 0;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void Base64Tool::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // Mode indicator
    drawCN(u8g2, 0, 22, encodeMode ? "模式:编码" : "模式:解码");
    u8g2.setFont(FONT_DATA);

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
        drawCN(u8g2, 0, 46, "输出:");
        u8g2.setFont(FONT_DATA);
        // Truncate to fit
        char outDisp[22];
        if (outputLen > 16) {
            strncpy(outDisp, output, 15); outDisp[15] = '~'; outDisp[16] = '\0';
        } else {
            strCopySafe(outDisp, output, sizeof(outDisp));
        }
        u8g2.drawStr(30, 46, outDisp);
    }

    // Footer
    u8g2.setFont(FONT_DATA);
}
