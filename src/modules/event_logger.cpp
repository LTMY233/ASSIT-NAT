#include "../chinese_glyphs.h"
#include "event_logger.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"

EventLogger* EventLogger::instance = nullptr;

ModuleInterface* createEventLogger() { return new EventLogger(); }

void EventLogger::init() {
    memset(entries, 0, sizeof(entries));
    entryCount = 0; cursor = 0; running = false;
    instance = this;
}

void EventLogger::enter() {
    entryCount = 0; cursor = 0; instance = this;
    wifiMgr.promiscuousAcquire(getId());
    running = true;
    addEntry("日志开始");
    displayMgr.setDirty();
}

void EventLogger::exit() {
    running = false;
    wifiMgr.promiscuousRelease(getId());
    instance = nullptr;
}

void EventLogger::addEntry(const char* msg) {
    if (entryCount >= EVT_LOG_MAX) {
        // shift
        for (uint8_t i = 0; i < EVT_LOG_MAX - 1; i++) {
            entries[i] = entries[i + 1];
        }
        entryCount = EVT_LOG_MAX - 1;
    }
    entries[entryCount].timestamp = millis();
    strCopySafe(entries[entryCount].message, msg, sizeof(entries[0].message));
    entryCount++;
}

void EventLogger::onPacket(uint8_t* buf, uint16_t len) {
    if (!instance || len < 24) return;

    uint8_t fc = buf[0];
    uint8_t type = fc & 0x0C;
    uint8_t subtype = (fc >> 4) & 0x0F;

    char macBuf[18]; macToStr(buf + 10, macBuf, sizeof(macBuf));

    if (type == 0x00 && subtype == 0x08) {  // Beacon
        char msg[48];
        uint8_t* body = buf + 24 + 12;
        char ssid[33] = "?";
        for (uint16_t i = 0; i < len - 40 && i < 200; ) {
            if (body[i] == 0 && body[i+1] <= 32) {
                memcpy(ssid, body + i + 2, body[i+1]);
                ssid[body[i+1]] = 0; break;
            }
            i += 2 + body[i+1];
        }
        snprintf(msg, sizeof(msg), "信标 %s", ssid);
        instance->addEntry(msg);
    } else if (type == 0x00 && subtype == 0x04) {  // Probe Req
        char msg[48];
        snprintf(msg, sizeof(msg), "探测 %s", macBuf);
        instance->addEntry(msg);
    } else if (type == 0x00 && subtype == 0x0C) {  // Deauth
        char msg[48];
        snprintf(msg, sizeof(msg), "去认证 %s", macBuf);
        instance->addEntry(msg);
    }
}

void EventLogger::update() {
    if (!running) return;
}

void EventLogger::handleButton(ButtonEvent ev) {
    if (ev == BTN_UP_SHORT && cursor > 0) { cursor--; displayMgr.setDirty(); }
    if (ev == BTN_DOWN_SHORT && cursor < entryCount - 1) { cursor++; displayMgr.setDirty(); }
    if (ev == BTN_OK_SHORT) { entryCount = 0; cursor = 0; displayMgr.setDirty(); }
}

void EventLogger::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    if (entryCount == 0) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 10, 35, "等待中...");
        return;
    }

    u8g2.setFont(FONT_DATA);
    uint8_t show = min((uint8_t)2, entryCount);
    for (uint8_t i = 0; i < show; i++) {
        uint8_t idx = (cursor >= show) ? entryCount - show + i : i;
        uint8_t y = 24 + i * 15;
        if (idx == cursor) {
            u8g2.drawBox(0, y - 12, OLED_WIDTH, 15);
            u8g2.setDrawColor(0);
        }
        char timeBuf[10];
        snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu",
                 (entries[idx].timestamp / 60000) % 60,
                 (entries[idx].timestamp / 1000) % 60);
        u8g2.drawStr(0, y, timeBuf);
        drawCN(u8g2, 40, y, entries[idx].message);
        u8g2.setFont(FONT_DATA);
        if (idx == cursor) u8g2.setDrawColor(1);
    }

    drawCN(u8g2, 0, 63, "按OK清除");
}
