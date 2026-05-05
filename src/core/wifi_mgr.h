#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <stdint.h>

enum WifiModeState : uint8_t {
    WM_OFF,
    WM_STA_CONNECTED,
    WM_STA_IDLE,
    WM_PROMISCUOUS,
    WM_AP,
};

class WifiMgr {
public:
    void init();

    // Mode switch
    bool setStationMode();
    bool connectToAP(const char* ssid, const char* pass);
    void disconnect();

    // Promiscuous (refcounted)
    bool promiscuousAcquire(uint8_t moduleId);
    void promiscuousRelease(uint8_t moduleId);
    int8_t promiscuousRefCount() const { return promiscRefs; }
    uint8_t promiscuousOwner() const { return promiscOwner; }

    // SoftAP mode
    bool startSoftAP(const char* ssid, const char* pass = nullptr, uint8_t channel = 1);
    void stopSoftAP();

    // Channel switch
    void setChannel(uint8_t ch);

    // Status query
    WifiModeState getState() const { return state; }
    bool isConnected() const;
    IPAddress localIP() const;
    IPAddress gatewayIP() const;
    IPAddress subnetMask() const;
    int8_t rssi() const;

    // Send raw 802.11 frame
    bool sendRawPacket(const uint8_t* frame, uint16_t len);

    // Force release (emergency)
    void forceRelease();

    // call each loop
    void update();

private:
    WifiModeState state;
    int8_t  promiscRefs;
    uint8_t promiscOwner;
    bool    apActive;

    void applyMode(WifiModeState newState);
};

extern WifiMgr wifiMgr;
