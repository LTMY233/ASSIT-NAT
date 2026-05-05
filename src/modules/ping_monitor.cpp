#include "ping_monitor.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

static WiFiUDP pingUdp;

ModuleInterface* createPingMonitor() { return new PingMonitor(); }

void PingMonitor::init() {
    currentRtt = -1;
    minRtt = 9999; maxRtt = 0;
    sumRtt = 0; pingCount = 0; successCount = 0;
    lossPercent = 0; lastRtt = 0;
    lastPing = 0; pingSent = false;
    pingStart = 0; pingSeq = 0;
    histIdx = 0;
    memset(rttHistory, -1, sizeof(rttHistory));
}

void PingMonitor::enter() {
    gateway = WiFi.gatewayIP();
    currentRtt = -1;
    minRtt = 9999; maxRtt = 0;
    sumRtt = 0; pingCount = 0; successCount = 0;
    lossPercent = 0; lastRtt = 0;
    lastPing = 0; pingSent = false;
    histIdx = 0;
    memset(rttHistory, -1, sizeof(rttHistory));
    pingUdp.begin(0);
    displayMgr.setDirty();
}

void PingMonitor::exit() {
    pingUdp.stop();
}

uint16_t PingMonitor::icmpChecksum(uint16_t* buf, int len) {
    uint32_t sum = 0;
    while (len > 1) { sum += *buf++; len -= 2; }
    if (len) sum += *(uint8_t*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

void PingMonitor::sendPing() {
    uint8_t packet[64] = {0};
    packet[0] = 8;  // Echo request
    packet[1] = 0;
    packet[4] = (pingSeq >> 8) & 0xFF;
    packet[5] = pingSeq & 0xFF;
    for (int i = 8; i < 40; i++) packet[i] = (uint8_t)i;

    uint16_t cs = icmpChecksum((uint16_t*)packet, 40);
    packet[2] = cs >> 8;
    packet[3] = cs & 0xFF;

    pingUdp.beginPacket(gateway, 0);
    pingUdp.write(packet, 40);
    pingUdp.endPacket();

    pingStart = millis();
    pingSent = true;
    pingSeq++;
}

bool PingMonitor::checkReply() {
    if (!pingUdp.parsePacket()) return false;
    uint8_t buf[64];
    int len = pingUdp.read(buf, 64);
    if (len < 20) return false;
    if (buf[0] != 0) return false;  // not echo reply

    int16_t rtt = millis() - pingStart;
    currentRtt = rtt;
    if (rtt < minRtt) minRtt = rtt;
    if (rtt > maxRtt) maxRtt = rtt;
    sumRtt += rtt;
    successCount++;
    pingCount++;

    // Record in history
    int8_t bar = rtt / 5;  // scale: 5ms per pixel
    if (bar > 40) bar = 40;
    if (bar < 1) bar = 1;
    rttHistory[histIdx] = bar;
    histIdx = (histIdx + 1) % 42;

    if (pingCount > 0) {
        lossPercent = (pingCount - successCount) * 100 / pingCount;
    }
    pingSent = false;
    return true;
}

void PingMonitor::update() {
    uint32_t now = millis();

    if (pingSent) {
        if (now - pingStart > 1000) {
            pingSent = false;
            pingCount++;
            if (pingCount > 0) {
                lossPercent = (pingCount - successCount) * 100 / pingCount;
            }
            currentRtt = -1;
            rttHistory[histIdx] = -1;
            histIdx = (histIdx + 1) % 42;
            displayMgr.setDirty();
        } else {
            if (checkReply()) displayMgr.setDirty();
        }
    }

    if (!pingSent && now - lastPing >= 1000) {
        lastPing = now;
        sendPing();
    }
}

void PingMonitor::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        minRtt = 9999; maxRtt = 0;
        sumRtt = 0; pingCount = 0; successCount = 0;
        lossPercent = 0;
        memset(rttHistory, -1, sizeof(rttHistory));
        histIdx = 0;
        displayMgr.setDirty();
    }
}

void PingMonitor::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Ping Monitor");

    // Gateway
    char gwBuf[20];
    ipToStr(gateway, gwBuf, sizeof(gwBuf));
    u8g2.setFont(FONT_BODY);
    u8g2.drawStr(2, 21, gwBuf);

    // Current RTT
    u8g2.setFont(FONT_BIG);
    char rttBuf[10];
    if (currentRtt >= 0) {
        snprintf(rttBuf, sizeof(rttBuf), "%dms", currentRtt);
    } else {
        strCopySafe(rttBuf, "--", sizeof(rttBuf));
    }
    uint8_t tw = u8g2.getStrWidth(rttBuf);
    u8g2.drawStr(OLED_WIDTH - tw - 2, 28, rttBuf);

    // Bar chart (text-based)
    u8g2.setFont(FONT_SMALL);
    for (uint8_t i = 0; i < 42; i++) {
        int8_t h = rttHistory[i];
        uint8_t x = i * 3;
        if (h >= 0) {
            for (uint8_t j = 0; j < h && j < 8; j++) {
                u8g2.drawPixel(x, 54 - j);
                u8g2.drawPixel(x + 1, 54 - j);
            }
        }
    }
    u8g2.drawHLine(0, 55, OLED_WIDTH);

    // Stats
    u8g2.setFont(FONT_DATA);
    int16_t avgRtt = successCount > 0 ? sumRtt / successCount : 0;
    char statBuf[48];
    snprintf(statBuf, sizeof(statBuf), "m:%d M:%d A:%d L:%d%%",
             minRtt == 9999 ? 0 : minRtt, maxRtt, avgRtt, lossPercent);
    u8g2.drawStr(0, 63, statBuf);
}
