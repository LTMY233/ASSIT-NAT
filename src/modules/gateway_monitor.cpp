#include "gateway_monitor.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include "../chinese_glyphs.h"

static WiFiUDP pingUdp;

ModuleInterface* createGatewayMonitor() { return new GatewayMonitor(); }

void GatewayMonitor::init() {
    currentRtt = -1;
    minRtt = 9999; maxRtt = 0; avgRtt = 0;
    totalPings = 0; successPings = 0;
    lossPercent = 0;
    jitter = 0;
    lastRtt = -1;
    lastPing = 0;
    pingSent = false;
    pingStart = 0;
    pingSeq = 0;
}

void GatewayMonitor::enter() {
    gateway = WiFi.gatewayIP();
    currentRtt = -1;
    minRtt = 9999; maxRtt = 0; avgRtt = 0;
    totalPings = 0; successPings = 0;
    lossPercent = 0; jitter = 0; lastRtt = -1;
    lastPing = 0;
    pingSent = false;
    pingUdp.begin(0);
    displayMgr.setDirty();
}

void GatewayMonitor::exit() {
    pingUdp.stop();
}

uint16_t GatewayMonitor::checksum(uint16_t* buf, int len) {
    uint32_t sum = 0;
    while (len > 1) { sum += *buf++; len -= 2; }
    if (len) sum += *(uint8_t*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

void GatewayMonitor::sendPing() {
    uint8_t packet[64] = {0};
    // ICMP Echo Request
    packet[0] = 8;  // type = Echo
    packet[1] = 0;  // code
    // checksum at [2:3]
    packet[4] = (pingSeq >> 8) & 0xFF;
    packet[5] = pingSeq & 0xFF;
    // payload
    for (int i = 8; i < 40; i++) packet[i] = (uint8_t)(i & 0xFF);

    uint16_t cs = checksum((uint16_t*)packet, 40);
    packet[2] = cs >> 8;
    packet[3] = cs & 0xFF;

    pingUdp.beginPacket(gateway, 0);  // raw IP
    pingUdp.write(packet, 40);
    pingUdp.endPacket();

    pingStart = millis();
    pingSent = true;
    pingSeq++;
}

bool GatewayMonitor::checkPingReply() {
    if (!pingUdp.parsePacket()) return false;
    uint8_t buf[64];
    int len = pingUdp.read(buf, 64);
    if (len < 20) return false;
    if (buf[0] != 0) return false;  // not echo reply

    currentRtt = millis() - pingStart;
    if (currentRtt < minRtt) minRtt = currentRtt;
    if (currentRtt > maxRtt) maxRtt = currentRtt;

    if (lastRtt >= 0) jitter = abs(currentRtt - lastRtt);
    lastRtt = currentRtt;

    avgRtt = (avgRtt * successPings + currentRtt) / (successPings + 1);
    successPings++;
    totalPings++;

    if (totalPings > 0) {
        lossPercent = (totalPings - successPings) * 100 / totalPings;
    }
    pingSent = false;
    return true;
}

void GatewayMonitor::update() {
    uint32_t now = millis();

    if (pingSent) {
        if (now - pingStart > 1000) {
            // timeout
            pingSent = false;
            totalPings++;
            if (totalPings > 0) {
                lossPercent = (totalPings - successPings) * 100 / totalPings;
            }
            currentRtt = -1;
            displayMgr.setDirty();
        } else {
            checkPingReply();
        }
    }

    if (!pingSent && now - lastPing >= 1000) {
        lastPing = now;
        sendPing();
    }
}

void GatewayMonitor::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        // reset stats
        minRtt = 9999; maxRtt = 0; avgRtt = 0;
        totalPings = 0; successPings = 0;
        lossPercent = 0; jitter = 0; lastRtt = -1;
        displayMgr.setDirty();
    }
}

void GatewayMonitor::draw(U8G2& u8g2) {
    drawCN(u8g2, 0, 10, "网关监控器");

    char gwBuf[20];
    ipToStr(gateway, gwBuf, sizeof(gwBuf));
    u8g2.setFont(FONT_BODY);
    u8g2.drawStr(2, 22, gwBuf);

    // Current latency (large)
    u8g2.setFont(FONT_BIG);
    char rttBuf[12];
    if (currentRtt >= 0) {
        snprintf(rttBuf, sizeof(rttBuf), "%dms", currentRtt);
    } else {
        strCopySafe(rttBuf, "--ms", sizeof(rttBuf));
    }
    uint8_t tw = u8g2.getStrWidth(rttBuf);
    u8g2.drawStr(OLED_WIDTH - tw - 2, 30, rttBuf);

    // Detailed stats
    u8g2.setFont(FONT_DATA);
    char statBuf[48];
    snprintf(statBuf, sizeof(statBuf), "min:%d  max:%d  avg:%d",
             minRtt == 9999 ? 0 : minRtt, maxRtt, avgRtt);
    u8g2.drawStr(2, 45, statBuf);

    snprintf(statBuf, sizeof(statBuf), "丢包:%d%%  抖动:%dms  总计:%d",
             lossPercent, jitter, totalPings);
    u8g2.drawStr(2, 63, statBuf);
}
