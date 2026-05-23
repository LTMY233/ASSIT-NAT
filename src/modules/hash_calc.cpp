#include "../chinese_glyphs.h"
#include "hash_calc.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

ModuleInterface* createHashCalc() { return new HashCalc(); }

char HashCalc::cycleChar(char c, bool up) {
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

void HashCalc::init() {
    strCopySafe(input, "Hello World", sizeof(input));
    inputLen = strlen(input);
    editPos = 0;
    hashMode = 1;  // default FNV32
    hashStr[0] = '\0';
    computed = false;
}

void HashCalc::enter() {
    editPos = 0;
    computed = false;
    hashStr[0] = '\0';
    displayMgr.setDirty();
}

void HashCalc::exit() {}

uint32_t HashCalc::xorHash(const char* str, uint8_t len) {
    uint32_t hash = 0;
    for (uint8_t i = 0; i < len; i++) {
        hash ^= ((uint32_t)str[i]) << ((i % 4) * 8);
    }
    return hash;
}

uint32_t HashCalc::fnv32Hash(const char* str, uint8_t len) {
    const uint32_t FNV_PRIME = 0x01000193;
    const uint32_t FNV_BASIS = 0x811C9DC5;
    uint32_t hash = FNV_BASIS;
    for (uint8_t i = 0; i < len; i++) {
        hash ^= (uint8_t)str[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

void HashCalc::computeHash() {
    uint32_t h;
    if (hashMode == 0) {
        h = xorHash(input, inputLen);
    } else {
        h = fnv32Hash(input, inputLen);
    }
    snprintf(hashStr, sizeof(hashStr), "%08lX", (unsigned long)h);
    computed = true;
}

void HashCalc::update() {}

void HashCalc::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            computeHash();
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
            hashMode = (hashMode + 1) % 2;
            computed = false;
            hashStr[0] = '\0';
            displayMgr.setDirty();
            break;
        default: break;
    }
}

void HashCalc::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // Mode
    drawCN(u8g2, 0, 22, hashMode == 0 ? "算法:XOR" : "算法:FNV32");
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

    // Hash output
    if (computed) {
        u8g2.setFont(FONT_DATA);
        drawCN(u8g2, 0, 48, "哈希:");
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(32, 48, hashStr);
    }

    u8g2.setFont(FONT_DATA);
}
