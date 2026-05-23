#include "../chinese_glyphs.h"
#include "channel_quality.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>

ModuleInterface* createChannelQuality() { return new ChannelQuality(); }

void ChannelQuality::init() {
    memset(channels, 0, sizeof(channels));
    for (uint8_t i = 0; i < 13; i++) channels[i].channel = i + 1;
    bestChannel = 0; hasData = false; lastScan = 0;
}

void ChannelQuality::enter() {
    hasData = false; lastScan = 0;
    doScan();
    displayMgr.setDirty();
}

void ChannelQuality::exit() { WiFi.scanDelete(); }

void ChannelQuality::doScan() {
    WiFi.scanNetworks(true, false);
    lastScan = millis();
}

void ChannelQuality::calcScores() {
    int n = WiFi.scanComplete();
    if (n < 0) return;
    WiFi.scanDelete();

    // Count per channel
    int8_t chCount[14] = {0};
    int32_t chRssiSum[14] = {0};

    for (int i = 0; i < n; i++) {
        uint8_t ch = WiFi.channel(i);
        if (ch >= 1 && ch <= 13) {
            chCount[ch]++;
            chRssiSum[ch] += WiFi.RSSI(i);
        }
    }

    // Scoring
    for (uint8_t ch = 1; ch <= 13; ch++) {
        channels[ch - 1].apCount = chCount[ch];
        channels[ch - 1].avgRssi = (chCount[ch] > 0) ? (chRssiSum[ch] / chCount[ch]) : 0;

        int8_t score = 100;
        // AP count penalty
        score -= chCount[ch] * 5;
        // Adjacent channel overlap penalty
        if (ch > 1) score -= chCount[ch - 1] * 2;
        if (ch < 13) score -= chCount[ch + 1] * 2;
        // Co-channel strong signal penalty
        if (channels[ch - 1].avgRssi < -70) score -= 10;
        if (channels[ch - 1].avgRssi < -80) score -= 10;

        if (score < 0) score = 0;
        if (score > 100) score = 100;
        channels[ch - 1].score = score;
    }

    // Find best channel
    bestChannel = 1;
    for (uint8_t ch = 1; ch <= 13; ch++) {
        if (channels[ch - 1].score > channels[bestChannel - 1].score) {
            bestChannel = ch;
        }
    }

    hasData = true;
}

void ChannelQuality::update() {
    int n = WiFi.scanComplete();
    if (n >= 0) {
        calcScores();
        displayMgr.setDirty();
    } else if (n < 0 && millis() - lastScan > WIFI_SCAN_TIMEOUT) {
        WiFi.scanDelete();
    }
}

void ChannelQuality::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) { hasData = false; doScan(); displayMgr.setDirty(); }
}

void ChannelQuality::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    if (!hasData) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 10, 35, "扫描中...");
        return;
    }

    u8g2.setFont(FONT_DATA);

    // Recommended channel (large text)
    char buf[30];
    snprintf(buf, sizeof(buf), "选:CH%d (分:%d)", bestChannel, channels[bestChannel - 1].score);
    drawCN(u8g2, 2, 24, buf);

    // 13 channel score bars
    for (uint8_t i = 0; i < 13; i++) {
        uint8_t x = 2 + i * 9;
        uint8_t h = channels[i].score * 8 / 100;

        uint8_t baseY = 48;
        if (h > 0) {
            if (i + 1 == bestChannel) {
                u8g2.drawBox(x, baseY - h, 7, h);  // Best channel filled
            } else {
                u8g2.drawFrame(x, baseY - h, 7, h);
            }
        }
        char chBuf[3]; itoaSimple(i + 1, chBuf, 10);
        u8g2.drawStr(x, baseY + 8, chBuf);
    }

    drawCN(u8g2, 0, 63, "按OK重扫");
}
