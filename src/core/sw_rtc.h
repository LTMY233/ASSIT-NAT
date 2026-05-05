#pragma once
#include <Arduino.h>
#include <stdint.h>

class SwRTC {
public:
    void init();
    void update();      // call each loop

    uint32_t now();     // UTC epoch
    uint32_t localNow();// local epoch (UTC + offset)

    bool isSynced() const { return synced; }

    // Manually set time (offline calib)
    void setTime(uint32_t epoch);

    // Request NTP sync (async, polled)
    void requestSync();

    // Last successful sync epoch
    uint32_t lastSyncEpoch() const { return lastSync; }

private:
    uint32_t epochBase;     // epoch at last sync
    uint32_t millisBase;    // millis() at last sync
    uint32_t lastSync;      // last sync epoch
    uint32_t ntpStart;      // NTP req start millis
    uint32_t nextSyncCheck; // next sync check millis
    bool     synced;
    bool     syncPending;

    void doNtpRequest();
    bool checkNtpResponse();
};

extern SwRTC swRTC;
