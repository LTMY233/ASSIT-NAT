#include "sw_rtc.h"
#include "config.h"
#include "utils.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

SwRTC swRTC;
static WiFiUDP udp;

void SwRTC::init() {
    epochBase = 0;
    millisBase = 0;
    lastSync = 0;
    ntpStart = 0;
    synced = false;
    syncPending = false;
    nextSyncCheck = millis() + 5000;  // try sync 5s after boot

    // Try restoring last time from LittleFS
    uint32_t savedEpoch;
    if (lfsReadFile(RTC_CALIBRATION_FILE, (uint8_t*)&savedEpoch, sizeof(savedEpoch))) {
        epochBase = savedEpoch;
        millisBase = millis();
        synced = true;
        lastSync = savedEpoch;
    }
}

uint32_t SwRTC::now() {
    if (!synced) return 0;
    uint32_t elapsed = (millis() - millisBase) / 1000;
    return epochBase + elapsed;
}

uint32_t SwRTC::localNow() {
    uint32_t utc = now();
    if (utc == 0) return 0;
    return utc + UTC_OFFSET_SEC;
}

void SwRTC::setTime(uint32_t epoch) {
    epochBase = epoch;
    millisBase = millis();
    synced = true;
    lastSync = epoch;
    // Persist to LittleFS
    lfsWriteFile(RTC_CALIBRATION_FILE, (uint8_t*)&epoch, sizeof(epoch));
}

void SwRTC::requestSync() {
    if (syncPending) return;
    doNtpRequest();
}

void SwRTC::doNtpRequest() {
    if (WiFi.status() != WL_CONNECTED) return;

    uint8_t packet[48] = {0};
    packet[0] = 0b11100011;  // LI=0, VN=4, Mode=3 (client)

    udp.begin(NTP_PORT);
    udp.beginPacket(NTP_SERVER, NTP_PORT);
    udp.write(packet, 48);
    udp.endPacket();

    ntpStart = millis();
    syncPending = true;
}

bool SwRTC::checkNtpResponse() {
    if (!udp.parsePacket()) {
        if (millis() - ntpStart > NTP_TIMEOUT) {
            syncPending = false;
        }
        return false;
    }

    uint8_t packet[48];
    udp.read(packet, 48);

    // NTP timestamp: bytes 40-43 (seconds)
    uint32_t highWord = ((uint32_t)packet[40] << 24) | ((uint32_t)packet[41] << 16) |
                        ((uint32_t)packet[42] << 8)  | packet[43];
    // NTP epoch starts 1900-01-01, UNIX starts 1970-01-01
    // Difference = 2208988800 sec
    const uint32_t NTP_UNIX_DELTA = 2208988800UL;
    if (highWord < NTP_UNIX_DELTA) {
        syncPending = false;
        return false;
    }
    uint32_t epoch = highWord - NTP_UNIX_DELTA;

    setTime(epoch);
    syncPending = false;
    udp.stop();
    return true;
}

void SwRTC::update() {
    if (syncPending) {
        checkNtpResponse();
        return;
    }

    // Periodic resync
    if (WiFi.status() == WL_CONNECTED) {
        uint32_t nowMs = millis();
        if (!synced || (nowMs - nextSyncCheck) > (NTP_RESYNC_INTERVAL * 1000UL)) {
            nextSyncCheck = nowMs + (NTP_RESYNC_INTERVAL * 1000UL);
            doNtpRequest();
        }
    }

    // Drift compensated by periodic NTP resync
    //
}
