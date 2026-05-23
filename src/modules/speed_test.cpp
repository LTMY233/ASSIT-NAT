#include "../chinese_glyphs.h"
#include "speed_test.h"
#include "../config.h"
#include "../core/display_mgr.h"
#include "../utils.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

ModuleInterface* createSpeedTest() { return new SpeedTest(); }

void SpeedTest::init() {
    running = false;
    done = false;
    speedKBps = 0;
    bytesDownloaded = 0;
    startTime = 0;
    elapsedMs = 0;
    wifiConnected = false;
}

void SpeedTest::enter() {
    running = false;
    done = false;
    speedKBps = 0;
    bytesDownloaded = 0;
    startTime = 0;
    elapsedMs = 0;
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    displayMgr.setDirty();
}

void SpeedTest::exit() {
    running = false;
    WiFi.scanDelete();
}

void SpeedTest::connectWiFi() {
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    if (!wifiConnected) {
        // Try connecting using stored credentials
        WiFi.begin();
        uint32_t connStart = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - connStart < 8000) {
            delay(100);
        }
        wifiConnected = (WiFi.status() == WL_CONNECTED);
    }
}

void SpeedTest::startTest() {
    if (running || !wifiConnected) return;
    running = true;
    done = false;
    bytesDownloaded = 0;
    speedKBps = 0;
    elapsedMs = 0;

    // Use a smaller, faster URL for ESP8266
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(5000);
    http.begin(client, SPEED_TEST_URL);

    startTime = millis();
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        WiFiClient* stream = http.getStreamPtr();
        uint8_t buf[512];
        uint32_t readStart = millis();

        while (stream->connected() && millis() - readStart < 8000) {
            if (stream->available()) {
                int len = stream->read(buf, sizeof(buf));
                if (len > 0) bytesDownloaded += len;
                elapsedMs = millis() - startTime;
                if (elapsedMs > 500) {
                    speedKBps = (float)bytesDownloaded / (float)elapsedMs;
                }
                displayMgr.setDirty();
                yield();
            }
            // Timeout after enough data
            if (bytesDownloaded > 100000) break;
        }
        elapsedMs = millis() - startTime;
    } else {
        elapsedMs = millis() - startTime;
    }

    http.end();
    if (elapsedMs > 0 && bytesDownloaded > 0) {
        speedKBps = (float)bytesDownloaded / (float)elapsedMs;
    }
    done = true;
    running = false;
    displayMgr.setDirty();
}

void SpeedTest::stopTest() {
    running = false;
    displayMgr.setDirty();
}

void SpeedTest::update() {
    if (running && !done) {
        // The download is handled inline in startTest() for simplicity
        // In a real implementation you'd use non-blocking reads
        displayMgr.setDirty();
    }
}

void SpeedTest::handleButton(ButtonEvent ev) {
    if (ev == BTN_OK_SHORT) {
        if (!wifiConnected) {
            connectWiFi();
        }
        if (wifiConnected && !running) {
            done = false;
            running = true;
            elapsedMs = 0;
            bytesDownloaded = 0;
            speedKBps = 0;
            // Start test in next update to avoid blocking
            startTest();
        }
    } else if (ev == BTN_UP_SHORT && running) {
        stopTest();
        displayMgr.setDirty();
    }
}

void SpeedTest::draw(U8G2& u8g2) {
    u8g2.setFont(FONT_DATA);

    if (!wifiConnected) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 10, 30, "WiFi未连接");
        drawCN(u8g2, 10, 42, "按OK连接");
    } else if (running) {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 2, 28, "下载中...");

        // Progress bar
        uint8_t barW = bytesDownloaded * 100 / 100000;
        if (barW > 100) barW = 100;
        u8g2.drawFrame(14, 36, 100, 6);
        u8g2.drawBox(14, 36, barW, 6);

        char buf[32];
        snprintf(buf, sizeof(buf), "%.1fs %luB", elapsedMs / 1000.0f, bytesDownloaded);
        u8g2.drawStr(2, 50, buf);

    } else if (done) {
        u8g2.setFont(FONT_BIG);
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", speedKBps);
        uint8_t tw = u8g2.getStrWidth(buf);
        u8g2.drawStr((OLED_WIDTH - tw) / 2, 28, buf);

        u8g2.setFont(FONT_DATA);
        u8g2.drawStr((OLED_WIDTH - tw) / 2 + tw, 28, "kB/s");

        snprintf(buf, sizeof(buf), "%lu bytes %.1fs", bytesDownloaded, elapsedMs / 1000.0f);
        u8g2.drawStr(2, 42, buf);
    } else {
        u8g2.setFont(FONT_BODY);
        drawCN(u8g2, 10, 35, "按OK开始测试");
    }

    u8g2.setFont(FONT_DATA);
    if (done) {
    } else {
    }
}
