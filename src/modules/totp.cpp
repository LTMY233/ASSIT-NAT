#include "../chinese_glyphs.h"
#include "totp.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/sw_rtc.h"
#include "../utils.h"
#include <Hash.h>

const char* TotpGenerator::defaultSecret = "JBSWY3DPEHPK3PXP";  // demo

ModuleInterface* createTotpGenerator() { return new TotpGenerator(); }

void TotpGenerator::init() {
    lastTotp = 0; lastCalc = 0;
    code[0] = '\0'; codeLen = 6;
    countdown = 30;
}

void TotpGenerator::enter() {
    lastTotp = 0; lastCalc = 0;
    countdown = 30;
    code[0] = '\0';
    displayMgr.setDirty();
}

void TotpGenerator::exit() {}

uint32_t TotpGenerator::base32Decode(const char* base32, uint8_t* out, uint8_t maxLen) {
    static const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    uint8_t buf[64] = {0};
    uint8_t bufLen = 0;
    uint16_t bits = 0;
    uint8_t bitCount = 0;

    while (*base32 && bufLen < maxLen) {
        const char* p = strchr(alphabet, toupper(*base32));
        if (!p) { base32++; continue; }
        bits = (bits << 5) | (p - alphabet);
        bitCount += 5;
        if (bitCount >= 8) {
            bitCount -= 8;
            buf[bufLen++] = (bits >> bitCount) & 0xFF;
        }
        base32++;
    }
    memcpy(out, buf, bufLen);
    return bufLen;
}

void TotpGenerator::hmacSha1(const uint8_t* key, uint8_t keyLen,
                              const uint8_t* msg, uint8_t msgLen, uint8_t* digest) {
    uint8_t ipad[64], opad[64];
    memset(ipad, 0x36, 64);
    memset(opad, 0x5C, 64);
    for (uint8_t i = 0; i < keyLen; i++) {
        ipad[i] ^= key[i];
        opad[i] ^= key[i];
    }

    uint8_t inner[20];
    uint8_t buf[128];
    memcpy(buf, ipad, 64);
    memcpy(buf + 64, msg, msgLen);
    sha1(buf, 64 + msgLen, inner);

    memcpy(buf, opad, 64);
    memcpy(buf + 64, inner, 20);
    sha1(buf, 84, digest);
}

void TotpGenerator::calcTotp() {
    uint32_t epoch = swRTC.now();
    if (epoch == 0) {
        strCopySafe(code, "No time", sizeof(code));
        return;
    }

    uint32_t counter = epoch / 30;  // 30-sec step
    countdown = 30 - (epoch % 30);

    // Skip duplicate calc
    if (counter == lastTotp) return;
    lastTotp = counter;

    // Decode seed
    uint8_t key[64];
    uint8_t keyLen = base32Decode(defaultSecret, key, sizeof(key));
    if (keyLen == 0) {
        strCopySafe(code, "Bad seed", sizeof(code));
        return;
    }

    // Build counter (big-endian)
    uint8_t msg[8] = {0};
    for (int i = 7; i >= 0; i--) {
        msg[i] = counter & 0xFF;
        counter >>= 8;
    }

    // HMAC-SHA1
    uint8_t digest[20];
    hmacSha1(key, keyLen, msg, 8, digest);

    // Dynamic truncation
    uint8_t offset = digest[19] & 0x0F;
    uint32_t binCode = ((digest[offset] & 0x7F) << 24) |
                       ((digest[offset + 1] & 0xFF) << 16) |
                       ((digest[offset + 2] & 0xFF) << 8) |
                       (digest[offset + 3] & 0xFF);

    // Mod
    uint32_t mod = 1;
    for (uint8_t i = 0; i < codeLen; i++) mod *= 10;
    binCode %= mod;

    snprintf(code, sizeof(code), "%0*lu", codeLen, (unsigned long)binCode);
}

void TotpGenerator::update() {
    calcTotp();
    if (code[0]) displayMgr.setDirty();
}

void TotpGenerator::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT && codeLen < 8) { codeLen++; lastTotp = 0; displayMgr.setDirty(); }
    if (ev == BTN_DOWN_SHORT && codeLen > 6) { codeLen--; lastTotp = 0; displayMgr.setDirty(); }
}

void TotpGenerator::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    // Code (large centered)
    u8g2.setFont(FONT_BIG);
    uint8_t tw = u8g2.getStrWidth(code);
    u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, code);

    // Countdown bar
    u8g2.drawFrame(20, 34, 88, 6);
    uint8_t prog = countdown * 88 / 30;
    u8g2.drawBox(20, 34, prog, 6);

    u8g2.setFont(FONT_DATA);
    char buf[30];
    snprintf(buf, sizeof(buf), "%ds  %d digits", countdown, codeLen);
    u8g2.drawStr(2, 45, buf);

    if (!swRTC.isSynced()) {
        drawCN(u8g2, 2, 63, "RTC未同步");
    } else {
    }
}
