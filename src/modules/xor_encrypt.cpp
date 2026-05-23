#include "../chinese_glyphs.h"
#include "xor_encrypt.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createXorEncrypt() { return new XorEncrypt(); }

char XorEncrypt::cycleChar(char c, bool up) {
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

void XorEncrypt::init() {
    strCopySafe(key, "secret", sizeof(key));
    keyLen = strlen(key);
    strCopySafe(input, "Hello World", sizeof(input));
    inputLen = strlen(input);
    editField = 0;  // 0=key, 1=input
    editPos = 0;
    output[0] = '\0';
    outputLen = 0;
    computed = false;
}

void XorEncrypt::enter() {
    editField = 0;
    editPos = 0;
    computed = false;
    output[0] = '\0';
    outputLen = 0;
    displayMgr.setDirty();
}

void XorEncrypt::exit() {}

void XorEncrypt::compute() {
    outputLen = inputLen;
    for (uint8_t i = 0; i < inputLen && i < sizeof(output) - 1; i++) {
        char k = key[i % keyLen];
        output[i] = input[i] ^ k;
        // Keep printable for display
        if (output[i] < 32 || output[i] > 126) {
            // Output as hex representation
        }
    }
    output[outputLen] = '\0';
    computed = true;
}

void XorEncrypt::update() {}

void XorEncrypt::handleButton(ButtonEvent ev) {
    char* buf = (editField == 0) ? key : input;
    uint8_t* lenPtr = (editField == 0) ? &keyLen : &inputLen;
    uint8_t maxLen = (editField == 0) ? 15 : 47;

    switch (ev) {
        case BTN_OK_SHORT:
            compute();
            displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            buf[editPos] = cycleChar(buf[editPos], true);
            computed = false;
            displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            buf[editPos] = cycleChar(buf[editPos], false);
            computed = false;
            displayMgr.setDirty();
            break;
        case BTN_OK_LONG:
            editPos = (editPos + 1) % (*lenPtr);
            displayMgr.setDirty();
            break;
        case BTN_UP_DOUBLE:
            editField = (editField + 1) % 2;
            editPos = 0;
            displayMgr.setDirty();
            break;
        case BTN_DOWN_DOUBLE:
            if (*lenPtr < maxLen) {
                buf[*lenPtr] = ' ';
                buf[*lenPtr + 1] = '\0';
                editPos = *lenPtr;
                (*lenPtr)++;
            }
            computed = false;
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void XorEncrypt::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // Key field
    u8g2.setFont(FONT_DATA);
    char kbuf[28];
    snprintf(kbuf, sizeof(kbuf), "密钥:%s%s", key, editField == 0 ? "_" : "");
    drawCN(u8g2, 0, 24, kbuf);
    u8g2.setFont(FONT_DATA);

    // Input field
    char ibuf[22];
    if (inputLen > 17) {
        strncpy(ibuf, input, 16); ibuf[16] = '~'; ibuf[17] = '\0';
    } else {
        strCopySafe(ibuf, input, sizeof(ibuf));
    }
    char lineBuf[28];
    snprintf(lineBuf, sizeof(lineBuf), "输入:%s%s", ibuf, editField == 1 ? "_" : "");
    drawCN(u8g2, 0, 36, lineBuf);
    u8g2.setFont(FONT_DATA);

    u8g2.drawHLine(0, 40, OLED_WIDTH);

    // Output
    if (computed) {
        u8g2.setFont(FONT_DATA);
        drawCN(u8g2, 0, 48, "输出:");
        u8g2.setFont(FONT_DATA);
        // Show as hex if output has non-printable chars
        char hexOut[40];
        uint8_t hIdx = 0;
        for (uint8_t i = 0; i < outputLen && hIdx < 36; i++) {
            hIdx += snprintf(hexOut + hIdx, sizeof(hexOut) - hIdx, "%02X", (uint8_t)output[i]);
        }
        hexOut[hIdx] = '\0';
        u8g2.drawStr(30, 48, hexOut);
    }

    u8g2.setFont(FONT_DATA);
}
