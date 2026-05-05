#include "wifi_mgr.h"
#include "config.h"
#include <ESP8266WiFi.h>

WifiMgr wifiMgr;

// Global promisc callback - dispatched by wifi_mgr
static void (*g_promiscCallback)(uint8_t*, uint16_t) = nullptr;

void wifi_promiscuous_rx_cb_wrapper(uint8_t* buf, uint16_t len) {
    if (g_promiscCallback) {
        g_promiscCallback(buf, len);
    }
}

void WifiMgr::init() {
    state = WM_OFF;
    promiscRefs = 0;
    promiscOwner = 0;
    apActive = false;
    WiFi.mode(WIFI_OFF);
}

bool WifiMgr::setStationMode() {
    if (state == WM_PROMISCUOUS || apActive) return false;
    WiFi.mode(WIFI_STA);
    state = WM_STA_IDLE;
    return true;
}

bool WifiMgr::connectToAP(const char* ssid, const char* pass) {
    if (state == WM_PROMISCUOUS) return false;
    WiFi.begin(ssid, pass);
    state = WM_STA_IDLE;
    // Connection result polled in update()
    return true;
}

void WifiMgr::disconnect() {
    WiFi.disconnect();
    state = WM_STA_IDLE;
}

bool WifiMgr::promiscuousAcquire(uint8_t moduleId) {
    if (promiscRefs == 0) {
        // First acquire: enable promiscuous mode
        WiFi.mode(WIFI_OFF);
        delay(10);
        wifi_set_opmode(STATION_MODE);
        wifi_promiscuous_enable(1);
        wifi_set_promiscuous_rx_cb(wifi_promiscuous_rx_cb_wrapper);
        state = WM_PROMISCUOUS;
        promiscOwner = moduleId;
    }
    promiscRefs++;
    return true;
}

void WifiMgr::promiscuousRelease(uint8_t moduleId) {
    if (promiscRefs <= 0) return;
    promiscRefs--;
    if (promiscRefs == 0) {
        wifi_promiscuous_enable(0);
        g_promiscCallback = nullptr;
        wifi_set_promiscuous_rx_cb(nullptr);
        state = WM_OFF;
        promiscOwner = 0;
    }
}

bool WifiMgr::startSoftAP(const char* ssid, const char* pass, uint8_t channel) {
    if (state == WM_PROMISCUOUS) {
        wifi_promiscuous_enable(0);
        state = WM_OFF;
    }
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, pass, channel);
    apActive = true;
    state = WM_AP;
    return true;
}

void WifiMgr::stopSoftAP() {
    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_OFF);
    apActive = false;
    state = WM_OFF;
}

void WifiMgr::setChannel(uint8_t ch) {
    if (ch < 1 || ch > 13) return;
    wifi_set_channel(ch);
}

void WifiMgr::forceRelease() {
    promiscuousRelease(promiscOwner);
    if (apActive) stopSoftAP();
    WiFi.mode(WIFI_OFF);
    state = WM_OFF;
    g_promiscCallback = nullptr;
}

bool WifiMgr::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

IPAddress WifiMgr::localIP() const {
    return WiFi.localIP();
}

IPAddress WifiMgr::gatewayIP() const {
    return WiFi.gatewayIP();
}

IPAddress WifiMgr::subnetMask() const {
    return WiFi.subnetMask();
}

int8_t WifiMgr::rssi() const {
    return WiFi.RSSI();
}

bool WifiMgr::sendRawPacket(const uint8_t* frame, uint16_t len) {
    if (state != WM_PROMISCUOUS) return false;
    return wifi_send_pkt_freedom((uint8_t*)frame, len, 0) == 0;
}

void WifiMgr::applyMode(WifiModeState newState) {
    switch (newState) {
        case WM_OFF:
            WiFi.mode(WIFI_OFF);
            break;
        case WM_STA_IDLE:
            WiFi.mode(WIFI_STA);
            break;
        case WM_PROMISCUOUS:
            wifi_promiscuous_enable(1);
            break;
        default:
            break;
    }
    state = newState;
}

void WifiMgr::update() {
    // WiFi event handling
    static wl_status_t lastStatus = WL_DISCONNECTED;
    wl_status_t current = WiFi.status();

    if (current != lastStatus) {
        lastStatus = current;
        if (current == WL_CONNECTED) {
            state = WM_STA_CONNECTED;
        } else {
            state = WM_STA_IDLE;
        }
    }
}
