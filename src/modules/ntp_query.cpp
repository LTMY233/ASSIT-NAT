#include "ntp_query.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"

static WiFiUDP ntpUdp;

ModuleInterface* createNtpQuery() { return new NtpQuery(); }

void NtpQuery::init() {
    ntpTime = 0;
    offset = 0;
    lastSync = 0;
    timeStr[0] = '\0';
    synced = false;
    querying = false;
    strCopySafe(status, "Unsynced", sizeof(status));
}

void NtpQuery::enter() {
    ntpTime = 0;
    offset = 0;
    lastSync = 0;
    timeStr[0] = '\0';
    synced = false;
    querying = false;
    strCopySafe(status, "OK=NTP sync", sizeof(status));
    ntpUdp.begin(0);
    displayMgr.setDirty();
}

void NtpQuery::exit() {
    ntpUdp.stop();
}

void NtpQuery::sendNtpQuery() {
    querying = true;
    strCopySafe(status, "Querying...", sizeof(status));
    displayMgr.setDirty();

    uint8_t packet[48] = {0};
    packet[0] = 0x1B;  // LI=0, VN=3, Mode=3 (client)

    IPAddress ntpServer;
    if (!WiFi.hostByName("pool.ntp.org", ntpServer)) {
        strCopySafe(status, "DNS Fail", sizeof(status));
        querying = false;
        displayMgr.setDirty();
        return;
    }

    ntpUdp.beginPacket(ntpServer, 123);
    ntpUdp.write(packet, 48);
    ntpUdp.endPacket();

    lastSync = millis();
}

bool NtpQuery::checkNtpReply() {
    if (!ntpUdp.parsePacket()) return false;
    uint8_t buf[48];
    int len = ntpUdp.read(buf, 48);
    if (len < 48) return false;

    uint32_t high = ((uint32_t)buf[40] << 24) | ((uint32_t)buf[41] << 16) |
                    ((uint32_t)buf[42] << 8) | buf[43];
    uint32_t low  = ((uint32_t)buf[44] << 24) | ((uint32_t)buf[45] << 16) |
                    ((uint32_t)buf[46] << 8) | buf[47];

    ntpTime = high - 2208988800UL;
    if (ntpTime < 1000000000UL) return false;

    uint32_t now = millis();
    offset = (int32_t)(ntpTime - now / 1000);

    formatTime(ntpTime, timeStr, sizeof(timeStr));
    synced = true;
    querying = false;
    snprintf(status, sizeof(status), "Off:%+lds", (long)offset);
    return true;
}

void NtpQuery::update() {
    if (querying) {
        if (millis() - lastSync > 3000) {
            strCopySafe(status, "NTP timeout", sizeof(status));
            querying = false;
            displayMgr.setDirty();
        } else if (checkNtpReply()) {
            displayMgr.setDirty();
        }
    }
}

void NtpQuery::handleButton(ButtonEvent ev) {
    switch (ev) {
        case BTN_OK_SHORT:
            sendNtpQuery();
            break;
        case BTN_UP_SHORT:
            if (synced && ntpTime > 0) {
                ntpTime += 3600;
                formatTime(ntpTime, timeStr, sizeof(timeStr));
                offset += 3600;
                snprintf(status, sizeof(status), "Off:%+lds", (long)offset);
                displayMgr.setDirty();
            }
            break;
        case BTN_DOWN_SHORT:
            if (synced && ntpTime > 3600) {
                ntpTime -= 3600;
                formatTime(ntpTime, timeStr, sizeof(timeStr));
                offset -= 3600;
                snprintf(status, sizeof(status), "Off:%+lds", (long)offset);
                displayMgr.setDirty();
            }
            break;
        default: break;
    }
}

void NtpQuery::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "NTP Query");

    if (synced) {
        // Time display
        u8g2.setFont(FONT_BIG);
        uint8_t tw = u8g2.getStrWidth(timeStr);
        u8g2.drawStr((OLED_WIDTH - tw) / 2, 30, timeStr);

        // Date
        u8g2.setFont(FONT_DATA);
        char dateBuf[16];
        formatDate(ntpTime, dateBuf, sizeof(dateBuf));
        u8g2.drawStr(10, 43, dateBuf);

        // Offset info
        u8g2.drawStr(0, 55, status);
    } else if (querying) {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(10, 35, "Querying NTP...");
        u8g2.drawStr(10, 46, "pool.ntp.org");
    } else {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(10, 35, "OK = Sync NTP");
        u8g2.drawStr(10, 46, "pool.ntp.org:123");
        u8g2.drawStr(10, 55, "UP/DN adjust TZ");
    }

    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 63, status);
}
