#include "../chinese_glyphs.h"
#include "frame_counter.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"

FrameCounter* FrameCounter::instance = nullptr;

ModuleInterface* createFrameCounter() { return new FrameCounter(); }

void FrameCounter::init() {
    mgmtCount = ctrlCount = dataCount = 0;
    beaconCount = probeReqCount = probeRespCount = 0;
    ackCount = rtsCount = ctsCount = 0;
    dataFrames = nullFrames = 0;
    mgmtRate = ctrlRate = dataRate = 0;
    totalPkts = 0;
    lastCalc = 0;
    running = false;
    instance = this;
}

void FrameCounter::enter() {
    mgmtCount = ctrlCount = dataCount = 0;
    beaconCount = probeReqCount = probeRespCount = 0;
    ackCount = rtsCount = ctsCount = 0;
    dataFrames = nullFrames = 0;
    mgmtRate = ctrlRate = dataRate = 0;
    totalPkts = 0;
    lastCalc = millis();
    instance = this;
    wifiMgr.promiscuousAcquire(getId());
    running = true;
    displayMgr.setDirty();
}

void FrameCounter::exit() {
    running = false;
    wifiMgr.promiscuousRelease(getId());
    instance = nullptr;
}

void FrameCounter::onPacket(uint8_t* buf, uint16_t len) {
    if (!instance || len < 2) return;

    uint8_t fc = buf[0];
    uint8_t type = fc & 0x0C;       // b2-b3
    uint8_t subtype = (fc >> 4) & 0x0F;

    if (type == 0x00) {  // Management
        instance->mgmtCount++;
        switch (subtype) {
            case 0x08: instance->beaconCount++; break;
            case 0x04: instance->probeReqCount++; break;
            case 0x05: instance->probeRespCount++; break;
        }
    } else if (type == 0x04) {  // Control
        instance->ctrlCount++;
        switch (subtype) {
            case 0x0D: instance->ackCount++; break;
            case 0x0B: instance->rtsCount++; break;
            case 0x0C: instance->ctsCount++; break;
        }
    } else if (type == 0x08) {  // Data
        instance->dataCount++;
        if (subtype == 0x00) instance->dataFrames++;
        if (subtype == 0x04) instance->nullFrames++;
    }

    instance->totalPkts++;
}

void FrameCounter::calcRates() {
    uint32_t now = millis();
    float elapsed = (now - lastCalc) / 1000.0f;
    if (elapsed <= 0) return;
    mgmtRate = (uint32_t)(mgmtCount / elapsed);
    ctrlRate = (uint32_t)(ctrlCount / elapsed);
    dataRate = (uint32_t)(dataCount / elapsed);
    // Don't reset counters, show cumulative
    lastCalc = now;
}

void FrameCounter::update() {
    if (!running) return;
    if (millis() - lastCalc >= 1000) {
        calcRates();
        displayMgr.setDirty();
    }
}

void FrameCounter::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        mgmtCount = ctrlCount = dataCount = 0;
        beaconCount = probeReqCount = probeRespCount = 0;
        ackCount = rtsCount = ctsCount = 0;
        dataFrames = nullFrames = 0;
        totalPkts = 0;
        lastCalc = millis();
        displayMgr.setDirty();
    }
}

void FrameCounter::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    u8g2.setFont(FONT_DATA);
    char buf[30];

    // Type stats
    snprintf(buf, sizeof(buf), "管:%lu/s 控:%lu/s 数:%lu/s", mgmtRate, ctrlRate, dataRate);
    drawCN(u8g2, 0, 24, buf);

    // Detailed breakdown
    snprintf(buf, sizeof(buf), "信标:%lu 探测:%lu/%lu 数:%lu 无:%lu", beaconCount, probeReqCount, probeRespCount, dataFrames, nullFrames);
    drawCN(u8g2, 0, 39, buf);

    snprintf(buf, sizeof(buf), "ACK:%lu RTS:%lu CTS:%lu", ackCount, rtsCount, ctsCount);
    u8g2.drawStr(0, 54, buf);

    snprintf(buf, sizeof(buf), "计:%lu  确认=重置", totalPkts);
    drawCN(u8g2, 0, 63, buf);
}
