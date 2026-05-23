#include "../chinese_glyphs.h"
#include "password_gen.h"
#include "../config.h"
#include "../core/display_mgr.h"

ModuleInterface* createPasswordGen() { return new PasswordGen(); }

void PasswordGen::init() {
    password[0] = '\0'; pwdLen = 16;
    useUpper = true; useLower = true; useDigits = true; useSymbols = true;
}

void PasswordGen::enter() {
    generate();
    displayMgr.setDirty();
}

void PasswordGen::exit() {}

void PasswordGen::generate() {
    const char* upper  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char* lower  = "abcdefghijklmnopqrstuvwxyz";
    const char* digits = "0123456789";
    const char* symbols = "!@#$%^&*()-_=+[]{}|;:,.<>?";

    char charset[80] = {0};
    uint8_t csLen = 0;

    if (useUpper)  { strcat(charset, upper); csLen += 26; }
    if (useLower)  { strcat(charset, lower); csLen += 26; }
    if (useDigits) { strcat(charset, digits); csLen += 10; }
    if (useSymbols){ strcat(charset, symbols); csLen += 24; }
    if (csLen == 0) { useLower = true; strcat(charset, lower); csLen = 26; }

    for (uint8_t i = 0; i < pwdLen; i++) {
        password[i] = charset[random() % csLen];
    }
    password[pwdLen] = '\0';

    // Ensure all selected types included
    if (useUpper)  password[0] = upper[random() % 26];
    if (useLower)  password[1] = lower[random() % 26];
    if (useDigits) password[2] = digits[random() % 10];
    if (useSymbols) password[3] = symbols[random() % 24];
}

void PasswordGen::update() {}

void PasswordGen::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            generate(); displayMgr.setDirty();
            break;
        case BTN_UP_SHORT:
            if (pwdLen < 32) pwdLen++;
            generate(); displayMgr.setDirty();
            break;
        case BTN_DOWN_SHORT:
            if (pwdLen > 8) pwdLen--;
            generate(); displayMgr.setDirty();
            break;
        case BTN_UP_DOUBLE:
            useUpper = !useUpper;
            generate(); displayMgr.setDirty();
            break;
        case BTN_DOWN_DOUBLE:
            useSymbols = !useSymbols;
            generate(); displayMgr.setDirty();
            break;
        default: break;
    }
}

void PasswordGen::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // Password (mono for readability)
    u8g2.setFont(FONT_MONO);
    if (pwdLen > 16) {
        // Split over lines
        char line1[17]; strncpy(line1, password, 16); line1[16] = '\0';
        u8g2.drawStr(2, 24, line1);
        u8g2.drawStr(2, 39, password + 16);
    } else {
        u8g2.drawStr(2, 30, password);
    }

    // Char set indicator + hint
    u8g2.setFont(FONT_DATA);
    char buf[60];
    snprintf(buf, sizeof(buf), "%d位 大写%c 小写%c 数字%c 符号%c",
             pwdLen,
             useUpper ? '+' : '-',
             useLower ? '+' : '-',
             useDigits ? '+' : '-',
             useSymbols ? '+' : '-');
    drawCN(u8g2, 0, 55, buf);
    drawCN(u8g2, 0, 63, "OK=生成");
}
